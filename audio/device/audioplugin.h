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

#ifndef AUDIO_MUSICPLUGIN_H
#define AUDIO_MUSICPLUGIN_H

#include "base/plugins.h"
#include "common/str.h"
#include "audio/device/device_intern.h"


namespace Common {
class Error;
}

class AudioPluginObject;

/**
 * Description of an audio device. Used to list the capabilities of the devices
 * that an audio plugin can manage. A device with an empty name refers to the default device.
 */
class AudioDevice {
public:
	AudioDevice(AudioPluginObject const *audioPlugin, Common::String name, SoundType type);

	Common::String &getDeviceName();
	Common::String &getPluginName();
	Common::String &getPluginId();
	//AudioPluginObject const *getPlugin() ;
	SoundType getSoundType();

	/**
	 * Returns a user readable string that contains the name of the current
	 * device (if it isn't the default one), the name of the plugin and the
	 * sound type description.
	 */
	Common::String getCompleteName();

	/**
	 * Returns a user readable string that contains the name of the current
	 * device (if it isn't the default one), the id of the plugin and the
	 * sound type description.
	 */
	Common::String getCompleteId();

	/**
	 * Provides a unique device identifier to  be passed on to the engine code.
	 */
	Audio::DeviceHandle getHandle() const;

	/**
	* Checks whether a device can actually be used. This call will be passed
	* on to the plugin. The actual check take place at plugin level.
	*/
	bool check();

	/*
	* Opens a port like interface to handle the communication between the engines
	* and the device. This should mostly behave like x86 in/out commands.
	*/
	Audio::Port *createPort();

private:
	Common::String _pluginName;
	Common::String _pluginId;
	Common::String _soundTypeDescription;
	Common::String _deviceName;
	AudioPluginObject const *_plugin;
	SoundType _soundType;
};

/** List of music devices. */
typedef Common::List<AudioDevice> AudioDevices;

/**
 * A AudioPluginObject is essentially a factory for MidiDriver instances with
 * the added ability of listing the available devices and their capabilities.
 */
class AudioPluginObject : public PluginObject {
public:
	virtual ~AudioPluginObject() {}

	/**
	 * Returns a unique string identifier which will be used to save the
	 * selected MIDI driver to the config file.
	 */
	virtual const char *getId() const = 0;

	/**
	 * Returns a list of the available devices.
	 */
	virtual AudioDevices getDevices() const = 0;

	/**
	 * Checks whether a device can actually be used. Currently this is only
	 * implemented for the MT-32 emulator to check whether the required rom
	 * files are present.
	 */
	virtual bool checkDevice(Audio::DeviceHandle) const { return true; }

	/**
	 * Tries to instantiate a MIDI Driver instance based on the device
	 * previously detected via MidiDriver::detectDevice()
	 *
	 * @param mididriver	Pointer to a pointer which the AudioPluginObject sets
	 *				to the newly create MidiDriver, or 0 in case of an error
	 *
	* @param dev	Pointer to a device to be used then creating the driver instance.
	 *				Default value of zero for driver types without devices.
	 *
	 * @return		a Common::Error describing the error which occurred, or kNoError
	 */
	//virtual Common::Error createInstance(MidiDriver **mididriver, MidiDriver::DeviceHandle = 0) const = 0;
	virtual Common::Error createPort(Audio::Port **port, Audio::DeviceHandle handle) const = 0;
};

/**
 * Singleton class which manages all Music plugins.
 */
class MusicManager : public Common::Singleton<MusicManager> {
private:
	friend class Common::Singleton<SingletonBaseType>;

public:
	const PluginList &getPlugins() const;
};

/** Convenience shortcut for accessing the Music manager. */
#define MusicMan MusicManager::instance()

#endif
