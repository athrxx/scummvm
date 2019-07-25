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

#ifndef TOWNS_AUDIO_H
#define TOWNS_AUDIO_H

#include "audio/device/porthandler.h"

namespace Audio {
class Mixer;
}

class TownsAudioInterfaceInternal;

class TownsAudioInterface : public Audio::PortHandler {
public:
	TownsAudioInterface(Audio::Mixer *mixer, Audio::TimerCallbackReceiver *driver);
	~TownsAudioInterface();

	enum ErrorCode {
		kSuccess = 0,
		kInvalidChannel,
		kUnavailable,
		kArgumentOutOfRange,
		kNotImplemented,
		kOutOfWaveMemory,
		kInvalidWaveTable,
		kChannelNotReserved,
		kNoteOutOfRangeForInstrument,
		kNoMatchingWaveTable,
		kDuplicateWaveTable
	};

	bool init();

	ErrorCode callback(int command, ...);

	void setMusicVolume(int volume);
	void setSoundEffectVolume(int volume);
	// Defines the channels used as sound effect channels for the purpose of ScummVM GUI volume control.
	// The first 6 bits are the 6 fm channels. The next 8 bits are pcm channels.
	void setSoundEffectChanMask(int mask);

	// PortHandler interface
	uint8 p_read(uint32 addr);
	void p_write(uint32 addr, uint8 val);
	int p_opcode(int command, va_list &args);
	int p_setCbReceiver(Audio::TimerCallbackReceiver *cb);
	int p_setMusicVolume(int vol);
	int p_setSfxVolume(int vol);
	int p_setSpeechVolume(int vol);
	int p_property(int prop, int value);

private:
	TownsAudioInterfaceInternal *_intf;
	uint8 _addrc[2];
};

#endif
