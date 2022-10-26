/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#if defined(USE_VST) && (defined(WIN32) || defined(WINDOWS) || defined(MACOSX))

#ifndef BACKENDS_VSTINTF_H
#define BACKENDS_VSTINTF_H

#include "audio/mididrv.h"

class VSTInterface {
protected:
	VSTInterface();
public:
	virtual ~VSTInterface();

	int open();
	void close();

	void send(uint32 msg);
	void update();

	//const char* getErrorDescription

protected:
private:
	bool _ready;

public:
	static VSTInterface *create(MidiDriver::DeviceHandle dev);
};

#endif

#endif
