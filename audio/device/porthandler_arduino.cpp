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

#include "audio/device/porthandler.h"
#include "audio/device/midi_receiver.h"

namespace Audio {

	PortHandler_Arduino::PortHandler_Arduino(SoundType soundType, MidiReceiver *mr) : PortHandler_Midi(soundType, mr) {
	_reg[0] = _reg[1] = 0;
}

void PortHandler_Arduino::p_write(uint32 addr, uint8 val) {
	addr &= 0x0F;
	uint8 cid = (addr >> 1) & 1;
	if (addr & 1) {
		_reg[cid] = val & 0x7F;
	} else {
		sendByte(0xB0 | cid);
		sendByte((_reg[cid] << 1) | ((val >> 7) & 1));
		sendByte(val & 0x7F);
	}
}

} // end of namespace Audio
