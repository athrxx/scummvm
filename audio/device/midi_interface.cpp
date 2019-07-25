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


#include "audio/device/device.h"

namespace Audio {

MidiInterface::MidiInterface() {
}

MidiInterface::~MidiInterface() {
}

void MidiInterface::sendMessage(uint8 evnt, uint8 para1, uint8 para2) {
	sendByte(evnt);
	sendByte(para1);
	if (para2)
		sendByte(para2);
}

void MidiInterface::sendMessage(uint32 msg) {
	sendByte(msg & 0xFF);
	msg >>= 8;
	sendByte(msg & 0xFF);
	msg >>= 8;
	if (msg & 0xFF)
		sendByte(msg & 0xFF);
}

void MidiInterface::sendSysex(const uint8 *bytes, uint32 len) {
	sendByte(0xF0);
	sendBytes(bytes, len);
	sendByte(0xF7);
}

void MidiInterface::metaEvent(byte type, byte *data, uint32 length) {
}
	
void MidiInterface::sendBytes(uint32 numBytes, ...) {
	va_list args;
	va_start(args, numBytes);
	while (numBytes--)
		sendByte(va_arg(args, uint8));
	va_end(args);
}

void MidiInterface::sendBytes(const uint8 *bytes, uint32 len) {
	if (!bytes || !len)
		return;
	while (len--)
		sendByte(*bytes++);
}

} // end of namespace Audio