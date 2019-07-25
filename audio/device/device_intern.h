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

#ifndef AUDIO_DEVICEINTERN_H
#define AUDIO_DEVICEINTERN_H

// This file provides declarations for all device functions that are required for the
// launcher or for the plugin and device code but needn't be exposed to the engines.

#ifndef AUDIO_DEVICE_H
#include "audio/device/device.h"
#endif

typedef Common::Array<SoundType> SoundTypes;

namespace Audio {

enum DeviceDescriptionType {
	kPluginName = 0,
	kPluginId,
	kDeviceName,
	kDeviceId
};

DeviceHandle getDeviceHandle(const Common::String &identifier);

SoundTypes enumHardwareSoundTypes();

const char *getSoundTypeDescription(SoundType type);

Common::String &getDeviceDescription(DeviceHandle handle, DeviceDescriptionType type);

bool checkDevice(DeviceHandle handle);

} // end of namespace Audio

#endif
