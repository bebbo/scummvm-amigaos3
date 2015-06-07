/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/scummsys.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "audio/audiostream.h"
#include "audio/decoders/raw.h"
#include "audio/decoders/3do.h"

#include "sherlock/scalpel/3do/movie_decoder.h"
#include "image/codecs/cinepak.h"

// for Test-Code
#include "common/system.h"
#include "common/events.h"
#include "common/keyboard.h"
#include "engines/engine.h"
#include "engines/util.h"
#include "graphics/palette.h"
#include "graphics/pixelformat.h"
#include "graphics/surface.h"

namespace Sherlock {

Scalpel3DOMovieDecoder::Scalpel3DOMovieDecoder()
	: _stream(0), _videoTrack(0), _audioTrack(0) {
	_streamVideoOffset = 0;
	_streamAudioOffset = 0;
}

Scalpel3DOMovieDecoder::~Scalpel3DOMovieDecoder() {
	close();
}

bool Scalpel3DOMovieDecoder::loadStream(Common::SeekableReadStream *stream) {
	uint32 videoSubType    = 0;
	uint32 videoCodecTag   = 0;
	uint32 videoHeight     = 0;
	uint32 videoWidth      = 0;
	uint32 videoFrameCount = 0;
	uint32 audioSubType    = 0;
	uint32 audioCodecTag   = 0;
	uint32 audioChannels   = 0;
	uint32 audioSampleRate = 0;

	close();

	_stream = stream;
	_streamVideoOffset = 0;
	_streamAudioOffset = 0;

	// Look for packets that we care about
	static const int maxPacketCheckCount = 20;
	for (int i = 0; i < maxPacketCheckCount; i++) {
		uint32 chunkTag = _stream->readUint32BE();
		uint32 chunkSize = _stream->readUint32BE() - 8;

		// Bail out if done
		if (_stream->eos())
			break;

		uint32 dataStartOffset = _stream->pos();

		switch (chunkTag) {
		case MKTAG('F','I','L','M'): {
			// See if this is a FILM header
			_stream->skip(4); // time stamp (based on 240 per second)
			_stream->skip(4); // Unknown 0x00000000
			videoSubType = _stream->readUint32BE();

			switch (videoSubType) {
			case MKTAG('F', 'H', 'D', 'R'):
				// FILM header found
				if (_videoTrack) {
					warning("Sherlock 3DO movie: Multiple FILM headers found");
					close();
					return false;
				}
				_stream->readUint32BE();
				videoCodecTag = _stream->readUint32BE();
				videoHeight = _stream->readUint32BE();
				videoWidth = _stream->readUint32BE();
				_stream->skip(4); // time scale
				videoFrameCount = _stream->readUint32BE();

				_videoTrack = new StreamVideoTrack(videoWidth, videoHeight, videoCodecTag, videoFrameCount);
				addTrack(_videoTrack);
				break;

			case MKTAG('F', 'R', 'M', 'E'):
				break;

			default:
				warning("Sherlock 3DO movie: Unknown subtype inside FILM packet");
				close();
				return false;
			}
			break;
		}

		case MKTAG('S','N','D','S'): {
			_stream->skip(8);
			audioSubType = _stream->readUint32BE();

			switch (audioSubType) {
			case MKTAG('S', 'H', 'D', 'R'):
				// Audio header

				// Bail if we already have a track
				if (_audioTrack) {
					warning("Sherlock 3DO movie: Multiple SNDS headers found");
					close();
					return false;
				}

				// OK, this is the start of a audio stream
				_stream->readUint32BE(); // Version, always 0x00000000
				_stream->readUint32BE(); // Unknown 0x00000008 ?!
				_stream->readUint32BE(); // Unknown 0x00007500
				_stream->readUint32BE(); // Unknown 0x00004000
				_stream->readUint32BE(); // Unknown 0x00000000
				_stream->readUint32BE(); // Unknown 0x00000010
				audioSampleRate = _stream->readUint32BE();			
				audioChannels = _stream->readUint32BE();
				audioCodecTag = _stream->readUint32BE();
				_stream->readUint32BE(); // Unknown 0x00000004 compression ratio?
				_stream->readUint32BE(); // Unknown 0x00000A2C

				_audioTrack = new StreamAudioTrack(audioCodecTag, audioSampleRate, audioChannels);
				addTrack(_audioTrack);
				break;

			case MKTAG('S', 'S', 'M', 'P'):
				// Audio data
				break;
			default:
				warning("Sherlock 3DO movie: Unknown subtype inside FILM packet");
				close();
				return false;
			}
			break;
		}

		case MKTAG('C','T','R','L'):
		case MKTAG('F','I','L','L'): // filler chunk, fills to certain boundary
		case MKTAG('D','A','C','Q'):
		case MKTAG('J','O','I','N'): // add cel data (not used in sherlock)
			// Ignore these chunks
			break;

		case MKTAG('S','H','D','R'):
			// Happens for EA logo, seems to be garbage data right at the start of the file
			break;

		default:
			warning("Unknown chunk-tag '%s' inside Sherlock 3DO movie", tag2str(chunkTag));
			close();
			return false;
		}

		if ((_videoTrack) && (_audioTrack))
			break;

		// Seek to next chunk
		_stream->seek(dataStartOffset + chunkSize);
	}

	// Bail if we didn't find video + audio
	if ((!_videoTrack) || (!_audioTrack)) {
		close();
		return false;
	}

	// Rewind back to the beginning
	_stream->seek(0);

	return true;
}

void Scalpel3DOMovieDecoder::close() {
	Video::VideoDecoder::close();

	delete _stream; _stream = 0;
	_videoTrack = 0;
}

// We try to at least decode 1 frame
// and also try to get at least 0.5 seconds of audio queued up
void Scalpel3DOMovieDecoder::readNextPacket() {
	uint32 currentMovieTime = getTime();
	uint32 wantedAudioQueued  = currentMovieTime + 500; // always try to be 0.500 seconds in front of movie time

	int32 chunkOffset     = 0;
	int32 dataStartOffset = 0;
	int32 nextChunkOffset = 0;
	uint32 chunkTag       = 0;
	uint32 chunkSize      = 0;

	uint32 videoSubType   = 0;
	uint32 videoTimeStamp = 0;
	uint32 videoFrameSize = 0;
	uint32 audioSubType   = 0;
	uint32 audioBytes     = 0;
	bool videoGotFrame = false;
	bool videoDone     = false;
	bool audioDone     = false;

	// Seek to smallest stream offset
	if (_streamVideoOffset <= _streamAudioOffset) {
		_stream->seek(_streamVideoOffset);
	} else {
		_stream->seek(_streamAudioOffset);
	}

	if (wantedAudioQueued <= _audioTrack->getTotalAudioQueued()) {
		// already got enough audio queued up
		audioDone = true;
	}

	while (1) {
		chunkOffset = _stream->pos();
		assert(chunkOffset >= 0);

		// Read chunk header
		chunkTag = _stream->readUint32BE();
		chunkSize = _stream->readUint32BE() - 8;

		// Calculate offsets
		dataStartOffset = _stream->pos();
		assert(dataStartOffset >= 0);
		nextChunkOffset = dataStartOffset + chunkSize;

		//warning("offset %lx - tag %lx", dataStartOffset, tag);

		if (_stream->eos())
			break;

		switch (chunkTag) {
		case MKTAG('F','I','L','M'):
			videoTimeStamp = _stream->readUint32BE();
			_stream->skip(4); // Unknown
			videoSubType = _stream->readUint32BE();

			switch (videoSubType) {
			case MKTAG('F', 'H', 'D', 'R'):
				// Ignore video header
				break;

			case MKTAG('F', 'R', 'M', 'E'):
				// Found frame data
				if (_streamVideoOffset <= chunkOffset) {
					// We are at an offset that is still relevant to video decoding
					if (!videoDone) {
						if (!videoGotFrame) {
							// We haven't decoded any frame yet, so do so now
							_stream->readUint32BE();
							videoFrameSize = _stream->readUint32BE();
							_videoTrack->decodeFrame(_stream->readStream(videoFrameSize), videoTimeStamp);

							_streamVideoOffset = nextChunkOffset;
							videoGotFrame = true;

						} else {
							// Already decoded a frame, so get timestamp of follow-up frame
							// and then we are done with video

							// Calculate next frame time
							// 3DO clock time for movies runs at 240Hh, that's why timestamps are based on 240.
							uint32 currentFrameStartTime = _videoTrack->getNextFrameStartTime();
							uint32 nextFrameStartTime = videoTimeStamp * 1000 / 240;
							assert(currentFrameStartTime <= nextFrameStartTime);
							_videoTrack->setNextFrameStartTime(nextFrameStartTime);

							// next time we want to start at the current chunk
							_streamVideoOffset = chunkOffset;
							videoDone = true;
						}
					}
				}
				break;

			default:
				error("Sherlock 3DO movie: Unknown subtype inside FILM packet");
				break;
			}
			break;

		case MKTAG('S','N','D','S'):
			_stream->skip(8);
			audioSubType = _stream->readUint32BE();

			switch (audioSubType) {
			case MKTAG('S', 'H', 'D', 'R'):
				// Ignore the audio header
				break;

			case MKTAG('S', 'S', 'M', 'P'):
				// Got audio chunk
				if (_streamAudioOffset <= chunkOffset) {
					// We are at an offset that is still relevant to audio decoding
					if (!audioDone) {
						audioBytes = _stream->readUint32BE();
						_audioTrack->queueAudio(_stream, audioBytes);

						_streamAudioOffset = nextChunkOffset;
						if (wantedAudioQueued <= _audioTrack->getTotalAudioQueued()) {
							// Got enough audio
							audioDone = true;
						}
					}
				}
				break;

			default:
				error("Sherlock 3DO movie: Unknown subtype inside SNDS packet");
				break;
			}
			break;

		case MKTAG('C','T','R','L'):
		case MKTAG('F','I','L','L'): // filler chunk, fills to certain boundary
		case MKTAG('D','A','C','Q'):
		case MKTAG('J','O','I','N'): // add cel data (not used in sherlock)
			// Ignore these chunks
			break;

		case MKTAG('S','H','D','R'):
			// Happens for EA logo, seems to be garbage data right at the start of the file
			break;

		default:
			error("Unknown chunk-tag '%s' inside Sherlock 3DO movie", tag2str(chunkTag));
		}

		// Always seek to end of chunk
		// Sometimes not all of the chunk is filled with audio
		_stream->seek(nextChunkOffset);

		if ((videoDone) && (audioDone)) {
			return;
		}
	}
}

Scalpel3DOMovieDecoder::StreamVideoTrack::StreamVideoTrack(uint32 width, uint32 height, uint32 codecTag, uint32 frameCount) {
	_width = width;
	_height = height;
	_frameCount = frameCount;
	_curFrame = -1;
	_nextFrameStartTime = 0;

	// Create the Cinepak decoder, if we're using it
	if (codecTag == MKTAG('c', 'v', 'i', 'd'))
		_codec = new Image::CinepakDecoder();
	else
		error("Unsupported Sherlock 3DO movie video codec tag '%s'", tag2str(codecTag));
}

Scalpel3DOMovieDecoder::StreamVideoTrack::~StreamVideoTrack() {
	delete _codec;
}

bool Scalpel3DOMovieDecoder::StreamVideoTrack::endOfTrack() const {
	return getCurFrame() >= getFrameCount() - 1;
}

Graphics::PixelFormat Scalpel3DOMovieDecoder::StreamVideoTrack::getPixelFormat() const {
	return _codec->getPixelFormat();
}

void Scalpel3DOMovieDecoder::StreamVideoTrack::decodeFrame(Common::SeekableReadStream *stream, uint32 videoTimeStamp) {
	_surface = _codec->decodeFrame(*stream);
	_curFrame++;
}

Scalpel3DOMovieDecoder::StreamAudioTrack::StreamAudioTrack(uint32 codecTag, uint32 sampleRate, uint32 channels) {
	switch (codecTag) {
	case MKTAG('A','D','P','4'):
	case MKTAG('S','D','X','2'):
		// ADP4 + SDX2 are both allowed
		break;

	default:
		error("Unsupported Sherlock 3DO movie audio codec tag '%s'", tag2str(codecTag));
	}

	_totalAudioQueued = 0; // currently 0 milliseconds queued

	_codecTag    = codecTag;
	_sampleRate  = sampleRate;
	switch (channels) {
	case 1:
		_stereo = false;
		break;
	case 2:
		_stereo = true;
		break;
	default:
		error("Unsupported Sherlock 3DO movie audio channels %d", channels);
	}

	_audioStream = Audio::makeQueuingAudioStream(sampleRate, _stereo);

	// reset audio decoder persistent spaces
	memset(&_ADP4_PersistentSpace, 0, sizeof(_ADP4_PersistentSpace));
	memset(&_SDX2_PersistentSpace, 0, sizeof(_SDX2_PersistentSpace));
}

Scalpel3DOMovieDecoder::StreamAudioTrack::~StreamAudioTrack() {
	delete _audioStream;
//	free(_ADP4_PersistentSpace);
//	free(_SDX2_PersistentSpace);
}

void Scalpel3DOMovieDecoder::StreamAudioTrack::queueAudio(Common::SeekableReadStream *stream, uint32 size) {
	Common::SeekableReadStream *compressedAudioStream = 0;
	Audio::RewindableAudioStream *audioStream = 0;
	uint32 audioLengthMSecs = 0;

	// Read the specified chunk into memory
	compressedAudioStream = stream->readStream(size);

	switch(_codecTag) {
	case MKTAG('A','D','P','4'):
		audioStream = Audio::make3DO_ADP4AudioStream(compressedAudioStream, _sampleRate, _stereo, &audioLengthMSecs, DisposeAfterUse::YES, &_ADP4_PersistentSpace);
		break;
	case MKTAG('S','D','X','2'):
		audioStream = Audio::make3DO_SDX2AudioStream(compressedAudioStream, _sampleRate, _stereo, &audioLengthMSecs, DisposeAfterUse::YES, &_SDX2_PersistentSpace);
		break;
	default:
		break;
	}
	if (audioStream) {
		_totalAudioQueued += audioLengthMSecs;
		_audioStream->queueAudioStream(audioStream, DisposeAfterUse::YES);
	}
}

Audio::AudioStream *Scalpel3DOMovieDecoder::StreamAudioTrack::getAudioStream() const {
	return _audioStream;
}

// Test-code

// Code for showing a movie. Only meant for testing/debug purposes
void Scalpel3DOMoviePlay(const char *filename) {
	// HACK
	initGraphics(320, 240, false, NULL);

	Scalpel3DOMovieDecoder *videoDecoder = new Scalpel3DOMovieDecoder();

	if (!videoDecoder->loadFile(filename)) {
		// HACK
		initGraphics(320, 200, false);
		return;
	}

	bool skipVideo = false;
	//byte bytesPerPixel = videoDecoder->getPixelFormat().bytesPerPixel;
	uint16 width = videoDecoder->getWidth();
	uint16 height = videoDecoder->getHeight();
	//uint16 pitch = videoDecoder->getWidth() * bytesPerPixel;

	videoDecoder->start();

	while (!g_engine->shouldQuit() && !videoDecoder->endOfVideo() && (!skipVideo)) {
		if (videoDecoder->needsUpdate()) {
			const Graphics::Surface *frame = videoDecoder->decodeNextFrame();

			if (frame) {
				g_system->copyRectToScreen(frame->getPixels(), frame->pitch, 0, 0, width, height);

				if (videoDecoder->hasDirtyPalette()) {
					const byte *palette = videoDecoder->getPalette();
					g_system->getPaletteManager()->setPalette(palette, 0, 255);
				}

				g_system->updateScreen();
			}
		}

		Common::Event event;
		while (g_system->getEventManager()->pollEvent(event)) {
			if ((event.type == Common::EVENT_KEYDOWN && event.kbd.keycode == Common::KEYCODE_ESCAPE))
				skipVideo = true;
		}

		g_system->delayMillis(10);
	}

	// HACK: switch back to 8bpp
	initGraphics(320, 200, false);
}


} // End of namespace Sherlock