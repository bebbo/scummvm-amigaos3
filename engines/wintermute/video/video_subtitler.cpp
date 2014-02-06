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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

/*
 * This file is based on Wintermute Engine
 * http://dead-code.org/redir.php?target=wme
 * Copyright (c) 2011 Jan Nedoma
 */

#include "engines/wintermute/video/video_subtitler.h"
#include "engines/wintermute/base/base_file_manager.h"
#include "engines/wintermute/utils/path_util.h"
#include "engines/wintermute/base/font/base_font.h"
#include "engines/wintermute/base/base_game.h"
#include "engines/wintermute/base/gfx/base_renderer.h"

namespace Wintermute {
//////////////////////////////////////////////////////////////////////////
VideoSubtitler::VideoSubtitler(BaseGame *inGame): BaseClass(inGame) {
	_lastSample = -1;
	_currentSubtitle = 0;
	_showSubtitle = false;
}

//////////////////////////////////////////////////////////////////////////
VideoSubtitler::~VideoSubtitler(void) {
	for (uint i = 0; i < _subtitles.size(); i++) {
		delete _subtitles[i];
	}

	_subtitles.clear();
}


//////////////////////////////////////////////////////////////////////////
bool VideoSubtitler::loadSubtitles(const char *filename, const char *subtitleFile) {
	if (!filename) {
		return false;
	}

	for (uint i = 0; i < _subtitles.size(); i++) {
		delete _subtitles[i];
	}

	_subtitles.clear();

	_lastSample = -1;
	_currentSubtitle = 0;
	_showSubtitle = false;


	Common::String newFile;

	if (subtitleFile) {
		newFile = Common::String(subtitleFile);
	} else {
		Common::String path = PathUtil::getDirectoryName(filename);
		Common::String name = PathUtil::getFileNameWithoutExtension(filename);
		Common::String ext = ".SUB";
		newFile = PathUtil::combine(path, name + ext);
	}

	long size;

	Common::SeekableReadStream *file = BaseFileManager::getEngineInstance()->openFile(newFile, true, false);

	if (file == nullptr) {
		return false; // no subtitles
	}

	size = file->size();
	char *buffer = new char[size];
	file->read(buffer, size);

	long start, end;
	bool inToken;
	char *tokenStart;
	int tokenLength;
	int tokenPos;
	int textLength;

	int pos = 0;
	int lineLength = 0;

	while (pos < size) {
		start = end = -1;
		inToken = false;
		tokenPos = -1;
		textLength = 0;

		lineLength = 0;

		while (pos + lineLength < size &&
		        buffer[pos + lineLength] != '\n' &&
		        buffer[pos + lineLength] != '\0') {
			lineLength++;
		}

		int realLength;

		if (pos + lineLength >= size) {
			realLength = lineLength - 0;
		} else {
			realLength = lineLength - 1;
		}

		char *text = new char[realLength + 1];
		char *line = (char *)&buffer[pos];

		for (int i = 0; i < realLength; i++) {
			if (line[i] == '{') {
				if (!inToken) {
					inToken = true;
					tokenStart = line + i + 1;
					tokenLength = 0;
					tokenPos++;
				} else {
					tokenLength++;
				}
			} else if (line[i] == '}') {
				if (inToken) {
					inToken = false;
					char *token = new char[tokenLength + 1];
					strncpy(token, tokenStart, tokenLength);
					token[tokenLength] = '\0';
					if (tokenPos == 0) {
						start = atoi(token);
					} else if (tokenPos == 1) {
						end = atoi(token);
					}
					delete[] token;
				} else {
					text[textLength] = line[i];
					textLength++;
				}
			} else {
				if (inToken) {
					tokenLength++;
				} else {
					text[textLength] = line[i];
					if (text[textLength] == '|') {
						text[textLength] = '\n';
					}
					textLength++;
				}
			}
		}

		text[textLength] = '\0';

		if (start != -1 && textLength > 0 && (start != 1 || end != 1)) {
			_subtitles.push_back(new VideoSubtitle(_gameRef, text, start, end));
		}

		delete [] text;

		pos += lineLength + 1;
	}

	delete[] buffer;
	// Succeeded loading subtitles!

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool VideoSubtitler::display() {
	if (_showSubtitle) {
		BaseFont *font = _gameRef->getVideoFont() ? _gameRef->getVideoFont() : _gameRef->getSystemFont();
		int textHeight = font->getTextHeight((byte *)_subtitles[_currentSubtitle]->_text, _gameRef->_renderer->getWidth());
		font->drawText((byte *)_subtitles[_currentSubtitle]->_text,
		               0,
		               (_gameRef->_renderer->getHeight() - textHeight - 5),
		               (_gameRef->_renderer->getWidth(), TAL_CENTER));
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool VideoSubtitler::update(long frame) {
	if (frame != _lastSample) {
		/*
		 * If the frame count hasn't advanced the previous state still matches
		 * the current frame (obviously).
		 */

		_lastSample = frame;
		// Otherwise, we update _lastSample; see above.

		_showSubtitle = false;

		bool overdue = (frame > _subtitles[_currentSubtitle]->_endFrame);
		bool hasNext = (_currentSubtitle + 1 < _subtitles.size());
		bool nextStarted = false;
		if (hasNext) {
			nextStarted = (_subtitles[_currentSubtitle + 1]->_startFrame <= frame);
		}

		while (_currentSubtitle < _subtitles.size() &&
		        overdue && hasNext && nextStarted) {
			/*
			 *  We advance until we get past all overdue subtitles.
			 *  We should exit the cycle when we either reach the first
			 *  subtitle which is not overdue whose subsequent subtitle
			 *  has not started yet (aka the one we must display now or
			 *  the one which WILL be displayed when its time comes)
			 *  and / or when we reach the last one.
			 */

			_currentSubtitle++;

			overdue = (frame > _subtitles[_currentSubtitle]->_endFrame);
			hasNext = (_currentSubtitle + 1 < _subtitles.size());
			if (hasNext) {
				nextStarted = (_subtitles[_currentSubtitle + 1]->_startFrame <= frame);
			} else {
				nextStarted = false;
			}
		}

		bool currentValid = (_subtitles[_currentSubtitle]->_endFrame != 0);
		/*
		 * No idea why we do this check, carried over from Mnemonic's code.
		 * Possibly a workaround for buggy subtitles or some kind of sentinel? :-\
		 */

		bool currentStarted = frame >= _subtitles[_currentSubtitle]->_startFrame;

		if (currentStarted && !overdue && currentValid) {
			_showSubtitle = true;
		}

	}
	return false;
}
}
