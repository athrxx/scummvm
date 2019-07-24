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

#include "audio/musicplugin.h"
#include "common/translation.h"
#include "common/error.h"
#include "common/system.h"


 // Plugin interface

class AdLibEmuMusicPlugin : public MusicPluginObject {
public:
	const char *getName() const {
		return _s("AdLib emulator");
	}

	const char *getId() const {
		return "adlib";
	}

	MusicDevices getDevices() const;
	Common::Error createInstance(MidiDriver **mididriver, MidiDriver::DeviceHandle = 0) const;
};

MusicDevices AdLibEmuMusicPlugin::getDevices() const {
	MusicDevices devices;
	devices.push_back(MusicDevice(this, "", MT_ADLIB));
	return devices;
}

Common::Error AdLibEmuMusicPlugin::createInstance(MidiDriver **mididriver, MidiDriver::DeviceHandle) const {
	*mididriver = 0;
	return Common::kNoError;
}

//#if PLUGIN_ENABLED_DYNAMIC(ADLIB)
	//REGISTER_PLUGIN_DYNAMIC(ADLIB, PLUGIN_TYPE_MUSIC, AdLibEmuMusicPlugin);
//#else
REGISTER_PLUGIN_STATIC(ADLIB, PLUGIN_TYPE_MUSIC, AdLibEmuMusicPlugin);
//#endif
