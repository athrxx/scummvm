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

#include "audio/device/audioplugin.h"
#include "common/error.h"

AudioDevice::AudioDevice(AudioPluginObject const *audioPlugin, Common::String name, SoundType type) :
	_plugin(audioPlugin), _pluginName(audioPlugin->getName()), _pluginId(audioPlugin->getId()), _deviceName(name), _soundType(type) {
}

Common::String &AudioDevice::getDeviceName() { 
	return _deviceName;
}

Common::String &AudioDevice::getPluginName() {
	return _pluginName;
}

Common::String &AudioDevice::getPluginId() {
	return _pluginId;
}

SoundType AudioDevice::getSoundType() {
	return _soundType;
}

Common::String AudioDevice::getCompleteName() {
	Common::String name = Audio::getSoundTypeDescription(_soundType);
	name += " --> ";

	if (_deviceName.empty()) {
		// Default device, just show the plugin name
		name += _plugin->getName();
	} else {
		// Show both device and plugin names
		name += _deviceName;
		name += " [";
		name += _plugin->getName();
		name += "]";
	}

	return name;
}

Common::String AudioDevice::getCompleteId() {
	Common::String id = _pluginId;
	if (!_deviceName.empty()) {
		id += "_";
		id += _deviceName;
	}

	id += "_";
	id += Audio::getSoundTypeDescription(_soundType);

	return id;
}

Audio::DeviceHandle AudioDevice::getHandle() const {
	return (Audio::DeviceHandle)this;
}

bool AudioDevice::check() {
	return _plugin->checkDevice(getHandle());
}

Audio::Port *AudioDevice::createPort() {
	Audio::Port *res = 0;
	Common::Error err = _plugin->createPort(&res, getHandle());

	if (err.getCode() != Common::kNoError)
		error("AudioDevice::createPort(): %s", err.getDesc().c_str());

	return res;
}

