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

PortHandler_Midi::PortHandler_Midi(SoundType soundType, MidiReceiver *mr) : PortHandler(soundType), _mr(mr), _pos(0) {
	assert(_mr);
	_buffer = new uint8[MIDI_MESSAGE_MAX_LENGTH];
}

PortHandler_Midi::~PortHandler_Midi() {
	delete _buffer;
}

uint8 PortHandler_Midi::p_read(uint32 addr) {
	addr &= 0x0F;
	return (addr == 1 && !isReady()) ? 0x40 : 0;
}

void PortHandler_Midi::p_write(uint32 addr, uint8 val) {
	addr &= 0x0F;
	if (addr == 0)
		sendByte(val);
	//else if (addr == 1)
}

void PortHandler_Midi::sendByte(uint8 b) {
	if (b == 0xF7 && _buffer[0] == 0xF0 && _pos) {
		_buffer[_pos++] = 0xF7;

		_mr->r_sysex(_buffer, _pos);

		_pos = 0;

	} else if (b >= 0x80) {
		_pos = 0;
		_buffer[_pos++] = b;

	} else if (_buffer[0] >= 0x80) {
		static const uint8 eventSize[] = { 2, 3, 2, 3, 2, 2, 3, 0 };
		uint8 cmd = (_buffer[0] - 0x80) >> 4;
		_buffer[_pos++] = b;
		if (_pos != eventSize[cmd])
			return;

		uint32 msg = _buffer[0] | (_buffer[1] << 8);
		if (eventSize[cmd] == 3)
			msg |= (_buffer[2] << 16);

		_mr->r_message(msg);

	} else {
		warning("PortHandler_Midi::sendByte(): Invalid Message");
		_pos = 0;
	}
}

//template<class RECEIVER>
bool PortHandler_Midi::isReady() const {
	return _mr->r_ready();
}

} // end of namespace Audio