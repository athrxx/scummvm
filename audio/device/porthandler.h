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

#ifndef AUDIO_PORTHANDLER_H
#define AUDIO_PORTHANDLER_H

#include "audio/device/device.h"
#include "common/scummsys.h"
#include "common/func.h"

#define	MIDI_MESSAGE_MAX_LENGTH		10000

namespace Audio {
// This basic port handler interface can be inherited by the emulators.
// The reads and writes can usually be passed on without any translation.
class PortHandler {
public:
	PortHandler(SoundType soundType) : _st(soundType) {}
	virtual ~PortHandler() {}
	// Mandatory methods
	virtual uint8 p_read(uint32 addr) = 0;
	virtual void p_write(uint32 addr, uint8 val) = 0;
	SoundType p_getType() const { return _st; }
	// Optional methods
	virtual int p_opcode(int command, va_list &args) { return -1; }
	virtual int p_setCbReceiver(Audio::TimerCallbackReceiver *cb) { return -1; }
	virtual int p_setMusicVolume(int vol) { return -1; }
	virtual int p_setSfxVolume(int vol) { return -1; }
	virtual int p_setSpeechVolume(int vol) { return -1; }
	virtual int p_property(int prop, int value) { return -1; }
private:
	SoundType _st;
};

// This port handler is for hardware devices and emulators that need to be addressed via Midi
// messages (basically all devices that would historically be connected through a MPU401
// interface or emulators of such devices. The port reads and writes get translated accordingly.
class MidiReceiver;
class PortHandler_Midi : public PortHandler {
public:
	PortHandler_Midi(SoundType soundType, MidiReceiver *mr);
	virtual ~PortHandler_Midi();
	virtual uint8 p_read(uint32 addr);
	virtual void p_write(uint32 addr, uint8 val);
protected:
	void sendByte(uint8 b);
	MidiReceiver *_mr;
	uint32 _pos;
private:
	bool isReady() const;	
	uint8 *_buffer;
};

// This port handler is for external FM devices controlled via an arduino board.
// The FM chip requires port reads and writes, but the arduino actually receives
// these commands encoded into (pseudo) Midi messages.
// Based on code by waltervn which he made for his own cms arduino device. The
// arduino has to be programmed in the same manner to be supported by this port
// handler.
class PortHandler_Arduino : public PortHandler_Midi {
public:
	PortHandler_Arduino(SoundType soundType, MidiReceiver *mr);
	virtual ~PortHandler_Arduino() {}
	void p_write(uint32 addr, uint8 val);
private:
	uint8 _reg[2];
};

} // end of namespace Audio

#endif
