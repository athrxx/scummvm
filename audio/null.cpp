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

#include "common/error.h"
#include "audio/null.h"

Common::Error NullAudioPlugin::createInstance(MidiDriver **mididriver, MidiDriver::DeviceHandle) const {
	*mididriver = new MidiDriver_NULL();

	return Common::kNoError;
}

MusicDevices NullAudioPlugin::getDevices() const {
	MusicDevices devices;
	devices.push_back(MusicDevice(this, "", MT_NULL));
	return devices;
}

class AutoAudioPlugin : public NullAudioPlugin {
public:
	const char *getName() const {
		return _s("<default>");
	}

	const char *getId() const {
		return "auto";
	}
	MusicDevices getDevices() const;
};

MusicDevices AutoAudioPlugin::getDevices() const {
	MusicDevices devices;
	devices.push_back(MusicDevice(this, "", MT_AUTO));
	return devices;
}

//#if PLUGIN_ENABLED_DYNAMIC(NULL)
	//REGISTER_PLUGIN_DYNAMIC(NULL, PLUGIN_TYPE_MUSIC, NullAudioPlugin);
//#else
	REGISTER_PLUGIN_STATIC(AUTO, PLUGIN_TYPE_MUSIC, AutoAudioPlugin);
	REGISTER_PLUGIN_STATIC(NULL, PLUGIN_TYPE_MUSIC, NullAudioPlugin);
//#endif
