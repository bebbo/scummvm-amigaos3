/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001/2002 The ScummVM project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 */

/*
 * Raw output support by Michael Pearce
 * MorphOS support by Ruediger Hanke 
 * Alsa support by Nicolas Noble <nicolas@nobis-crew.org> copied from
 *    both the QuickTime support and (vkeybd http://www.alsa-project.org/~iwai/alsa.html)
 */

#include "stdafx.h"
#include "mididrv.h"
#include "mpu401.h"
#include "common/engine.h"	// for warning/error/debug
#include "common/util.h"	// for ARRAYSIZE



/* Default (empty) property method */
uint32 MidiDriver::property(int prop, uint32 param)
{
	return 0;
}


/* retrieve a string representation of an error code */
const char *MidiDriver::getErrorName(int error_code)
{
	static const char *const midi_errors[] = {
		"No error",
		"Cannot connect",
		"Streaming not available",
		"Device not available",
		"Driver already open"
	};

	if ((uint) error_code >= ARRAYSIZE(midi_errors))
		return "Unknown Error";
	return midi_errors[error_code];
}

#ifdef __MORPHOS__
#include <exec/memory.h>
#include <exec/types.h>
#include <devices/etude.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/etude.h>

#include "morphos_sound.h"

/* MorphOS MIDI driver */
class MidiDriver_ETUDE : public MidiDriver_MPU401 {
public:
	MidiDriver_ETUDE();
	int open(int mode);
	void close();
	void send(uint32 b);

private:
	enum {
		NUM_BUFFERS = 2,
		MIDI_EVENT_SIZE = 64,
		BUFFER_SIZE = MIDI_EVENT_SIZE * 12,
	};

	uint32 property(int prop, uint32 param);

	bool _isOpen;
};

MidiDriver_ETUDE::MidiDriver_ETUDE()
{
	_isOpen = false;
}

int MidiDriver_ETUDE::open()
{
	if (_isOpen)
		return MERR_ALREADY_OPEN;
	_isOpen = true;
	if (!init_morphos_music(0, ETUDEF_DIRECT))
		return MERR_DEVICE_NOT_AVAILABLE;

	return 0;
}

void MidiDriver_ETUDE::close()
{
	exit_morphos_music();
	_isOpen = false;
}

void MidiDriver_ETUDE::send(uint32 b)
{
	if (_isOpen)
		error("MidiDriver_ETUDE::send called but driver was no opened");

	if (ScummMidiRequest) {
		ULONG midi_data = READ_LE_UINT32(&b);
		SendShortMidiMsg(ScummMidiRequest, midi_data);
	}
}

extern MidiDriver* EtudeMidiDriver = NULL;

MidiDriver *MidiDriver_ETUDE_create()
{
	if (!EtudeMidiDriver)
		EtudeMidiDriver = new MidiDriver_ETUDE();
	return EtudeMidiDriver;
}

#endif // __MORPHOS__

#if defined(UNIX) && !defined(__BEOS__)
#define SEQ_MIDIPUTC    5
#define SPECIAL_CHANNEL 9

class MidiDriver_SEQ : public MidiDriver_MPU401 {
public:
	MidiDriver_SEQ();
	int open();
	void close();
	void send(uint32 b);

private:
	bool _isOpen;
	int device, _device_num;
};

MidiDriver_SEQ::MidiDriver_SEQ()
{
	_isOpen = false;
	device = 0;
	_device_num = 0;
}

int MidiDriver_SEQ::open()
{
	if (_isOpen)
		return MERR_ALREADY_OPEN;
	_isOpen = true;
	device = 0;

	char *device_name = getenv("SCUMMVM_MIDI");
	if (device_name != NULL) {
		device = (::open((device_name), O_RDWR, 0));
	} else {
		warning("You need to set-up the SCUMMVM_MIDI environment variable properly (see README) ");
	}
	if ((device_name == NULL) || (device < 0)) {
		if (device_name == NULL)
			warning("Opening /dev/null (no music will be heard) ");
		else
			warning("Cannot open rawmidi device %s - using /dev/null (no music will be heard) ",
							device_name);
		device = (::open(("/dev/null"), O_RDWR, 0));
		if (device < 0)
			error("Cannot open /dev/null to dump midi output");
	}

	if (getenv("SCUMMVM_MIDIPORT"))
		_device_num = atoi(getenv("SCUMMVM_MIDIPORT"));
	return 0;
}

void MidiDriver_SEQ::close()
{
	::close(device);
	_isOpen = false;
}


void MidiDriver_SEQ::send(uint32 b)
{
	unsigned char buf[256];
	int position = 0;

	switch (b & 0xF0) {
	case 0x80:
	case 0x90:
	case 0xA0:
	case 0xB0:
	case 0xE0:
		buf[position++] = SEQ_MIDIPUTC;
		buf[position++] = (unsigned char)b;
		buf[position++] = _device_num;
		buf[position++] = 0;
		buf[position++] = SEQ_MIDIPUTC;
		buf[position++] = (unsigned char)((b >> 8) & 0x7F);
		buf[position++] = _device_num;
		buf[position++] = 0;
		buf[position++] = SEQ_MIDIPUTC;
		buf[position++] = (unsigned char)((b >> 16) & 0x7F);
		buf[position++] = _device_num;
		buf[position++] = 0;
		break;
	case 0xC0:
	case 0xD0:
		buf[position++] = SEQ_MIDIPUTC;
		buf[position++] = (unsigned char)b;
		buf[position++] = _device_num;
		buf[position++] = 0;
		buf[position++] = SEQ_MIDIPUTC;
		buf[position++] = (unsigned char)((b >> 8) & 0x7F);
		buf[position++] = _device_num;
		buf[position++] = 0;
		break;
	default:
		fprintf(stderr, "Unknown : %08x\n", (int)b);
		break;
	}
	write(device, buf, position);
}

MidiDriver *MidiDriver_SEQ_create()
{
	return new MidiDriver_SEQ();
}

#endif

