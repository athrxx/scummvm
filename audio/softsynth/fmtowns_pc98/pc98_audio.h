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

#ifndef PC98_AUDIO_H
#define PC98_AUDIO_H

#include "common/scummsys.h"
#include "audio/device/porthandler.h"

namespace Audio {
class Mixer;
}

class PC98AudioCoreInternal;

class PC98AudioCore : public Audio::PortHandler {
public:
	enum EmuType {
		kTypeFMTowns	= 0,
		kType980126		= 1,
		kType980186		= 2
	};

public:
	PC98AudioCore(Audio::Mixer *mixer, Audio::TimerCallbackReceiver *driver, EmuType type);
	~PC98AudioCore();

	bool init();
	void reset();

	void writeReg(uint8 part, uint8 regAddress, uint8 value);
	uint8 readReg(uint8 part, uint8 regAddress);

	void writePort(uint16 port, uint8 value);
	uint8 readPort(uint16 port);

	void setMusicVolume(int volume);
	void setSoundEffectVolume(int volume);

	// Defines the channels used as sound effect channels for the purpose of ScummVM GUI volume control.
	// The first 6 bits are the 6 fm channels. The next 3 bits are ssg channels. The next bit is the rhythm channel.
	void setSoundEffectChanMask(int mask);

	void ssgSetVolume(int volume);

	// PortHandler interface
	uint8 p_read(uint32 addr) { return readPort(addr & 0xFFFF); }
	void p_write(uint32 addr, uint8 val) { writePort(addr & 0xFFFF, val); }
	int p_setCbReceiver(Audio::TimerCallbackReceiver *cb);
	int p_setMusicVolume(int vol);
	int p_setSfxVolume(int vol);
	int p_property(int prop, int value);

private:
	PC98AudioCoreInternal *_internal;
	int _ssgVolume;
};

#endif
