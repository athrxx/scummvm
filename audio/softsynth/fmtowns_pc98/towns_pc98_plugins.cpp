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

#include "audio/softsynth/fmtowns_pc98/pc98_audio.h"
#include "audio/softsynth/fmtowns_pc98/towns_audio.h"
#include "audio/device/audioplugin.h"
#include "audio/device/porthandler.h"

#include "common/translation.h"
#include "common/error.h"
#include "common/system.h"


class TownsEmuAudioPlugin : public AudioPluginObject {
public:
	const char *getName() const {
		return _s("FM-Towns Audio");
	}

	const char *getId() const {
		return "towns";
	}

	AudioDevices getDevices() const;
	Common::Error createPort(Audio::Port **port, Audio::DeviceHandle = 0) const;
};

AudioDevices TownsEmuAudioPlugin::getDevices() const {
	AudioDevices devices;
	devices.push_back(AudioDevice(this, "", SND_TOWNS));
	return devices;
}

Common::Error TownsEmuAudioPlugin::createPort(Audio::Port **port, Audio::DeviceHandle handle) const {
	*port = new Audio::Port(new TownsAudioInterface(g_system->getMixer(), 0), false);
	return *port ? Common::kNoError : Common::kUnknownError;
}

class PC98EmuAudioPlugin : public AudioPluginObject {
public:
	const char *getName() const {
		return _s("PC-98 Audio");
	}

	const char *getId() const {
		return "pc98";
	}

	AudioDevices getDevices() const;
	Common::Error createPort(Audio::Port **port, Audio::DeviceHandle) const;
};

AudioDevices PC98EmuAudioPlugin::getDevices() const {
	AudioDevices devices;
	devices.push_back(AudioDevice(this, "", SND_PC98_86));
	return devices;
}

Common::Error PC98EmuAudioPlugin::createPort(Audio::Port **port, Audio::DeviceHandle dev) const {
	*port = new Audio::Port(new PC98AudioCore(g_system->getMixer(), 0, Audio::getDeviceSoundType(dev) == SND_PC98_86 ?
		PC98AudioPluginDriver::EmuType::kType86 : PC98AudioPluginDriver::EmuType::kType26), false);
	return *port ? Common::kNoError : Common::kUnknownError;
}

//#if PLUGIN_ENABLED_DYNAMIC(TOWNS)
	//REGISTER_PLUGIN_DYNAMIC(TOWNS, PLUGIN_TYPE_MUSIC, TownsEmuAudioPlugin);
	//REGISTER_PLUGIN_DYNAMIC(TOWNS, PLUGIN_TYPE_MUSIC, TownsEmuAudioPlugin);
//#else
	REGISTER_PLUGIN_STATIC(TOWNS, PLUGIN_TYPE_MUSIC, TownsEmuAudioPlugin);
	REGISTER_PLUGIN_STATIC(PC98, PLUGIN_TYPE_MUSIC, PC98EmuAudioPlugin);
//#endif
