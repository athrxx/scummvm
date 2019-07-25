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

#include "audio/device/device_intern.h"
/*#include "common/error.h"
#include "common/gui_options.h"
#include "common/str.h"
#include "common/system.h"
#include "common/textconsole.h"*/
#include "common/config-manager.h"
#include "common/util.h"
/*#include "gui/message.h"
#include "audio/mididrv.h"*/
#include "audio/device/audioplugin.h"
#include "audio/device/porthandler.h"

struct TypeDescription {
	SoundType type;
	const char *desc;
	bool isGlobalOption;	// Some devices (FB01, CSM-1, etc.) only have support on few engines. They needn't show up in the global options.
	bool isMidi;			// This decides whether a device for that sound type can have a midi interface.
	bool supportsHardware;	// This decides whether a device for that sound type can be an actual hardware device.
};

TypeDescription typeDesc[] {									// global	// midi		// hardware support
	{ SND_AUTO,		"Default",									true,		false,		false },
	{ SND_NULL,		"No Sound",									true,		false,		false },
	{ SND_DIGITAL,	"Digital Audio",							true,		false,		false },
	{ SND_PCSPK,	"PC Speaker",								true,		false,		false },
	{ SND_PCJR,		"PCJr",										true,		false,		false },
	{ SND_CMS,		"Creative Music System / Game Blaster",		true,		false,		true  },	// I enable hw support here for arduino cms devices
	{ SND_ADLIB,	"AdLiB / Sound Blaster 2.0",				true,		false,		false },
	{ SND_OPL3,		"Sound Blaster Pro 2 / Sound Blaster 16",	false,		false,		false },
	{ SND_DUALOPL,	"Sound Blaster Pro",						false,		false,		false },
	{ SND_GM,		"General Midi / Sound Canvas",				true,		true,		true },
	{ SND_MT32,		"Roland LAPC / MT-32 / CM32L",				true,		true,		true },
	{ SND_GS,		"Roland GS compatible",						false,		true,		true },		// TODO/CHECK: Is this actually needed anywhere?
	{ SND_D110,		"Roland D-110",								false,		true,		true },
	{ SND_FB01,		"Yamaha FB-01 / IBM Music Feature Card",	false,		true,		true },
	{ SND_CSM1,		"Casio CSM-1",								false,		true,		true },
	{ SND_C64,		"Commodore 64 Audio",						true,		false,		false },
	{ SND_AMIGA,	"Amiga Audio",								true,		false,		false },
	{ SND_MAC,		"Apple Macintosh Audio",					true,		false,		false },
	{ SND_APPLIIGS,	"Apple IIGS Audio",							true,		false,		false },
	{ SND_TOWNS,	"FM-Towns Audio",							true,		false,		false },
	{ SND_PC98_86,	"PC-9801-86 Audio",							true,		false,		false },
	{ SND_PC98_26,	"PC-9801-26 Audio",							true,		false,		false },
	{ SND_NES,		"Nintendo Ent. System / Famicom Audio",		true,		false,		false },
	{ SND_PCE,		"Nec PC-Engine Audio",						true,		false,		false }
};

namespace Audio {

struct PlatformSoundType {
	Common::Platform platform;
	SoundType types[13];
};
	
PlatformSoundType platformSoundTypes[] = {
	{ Common::kPlatformDOS,			{ SND_DIGITAL, SND_PCSPK, SND_PCJR, SND_CMS, SND_ADLIB, SND_OPL3, SND_DUALOPL, SND_GM, SND_MT32, SND_GS, SND_FB01, SND_CSM1, SND_D110 } },
	{ Common::kPlatformWindows,		{ SND_DIGITAL, SND_PCSPK, SND_PCJR, SND_CMS, SND_ADLIB, SND_OPL3, SND_DUALOPL, SND_GM, SND_MT32, SND_GS, SND_FB01, SND_CSM1, SND_D110 } },
	{ Common::kPlatformAmiga,		{ SND_AMIGA, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID } },
	{ Common::kPlatformMacintosh,	{ SND_DIGITAL, SND_MAC, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID } },
	{ Common::kPlatformFMTowns,		{ SND_DIGITAL, SND_TOWNS, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID } },
	{ Common::kPlatformPC98,		{ SND_DIGITAL, SND_PC98_86, SND_PC98_26, SND_GM, SND_MT32, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID } },
	{ Common::kPlatformNES,			{ SND_NES, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID } },
	{ Common::kPlatformC64,			{ SND_C64, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID } },
	{ Common::kPlatformPCEngine,	{ SND_PCE, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID } },
	{ Common::kPlatformApple2GS,	{ SND_APPLIIGS, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID, SND_VOID } }
};

//Common::Array<DeviceHandle> _devicehandles;

class MidiPortInterface : public MidiInterface {
public:
	MidiPortInterface(Port *port) : MidiInterface(), _port(port) {}
	virtual ~MidiPortInterface() {}

	void sendByte(uint8 b) {
		_port->write(0, 0x330);
		_port->write(1, b);
	}

private:
	Port *_port;
};

Port::Port(PortHandler *ph, bool provideMidiInterface) : _ph(ph), _flags(0), _midiInterface(provideMidiInterface ? new MidiPortInterface(this) : 0) {
	assert(ph);

	// Detect caps...
	if (_midiInterface)
		_flags |= kCAPS_MIDIINTERFACE;
	if (runOpcode(-1) != kRC_NOTIMPLEMENTED)
		_flags |= kCAPS_OPCODEINTERFACE;
	if (setTimerCallbackReceiver(0) != kRC_NOTIMPLEMENTED)
		_flags |= kCAPS_TIMERCALLBACK;
	if (setMusicVolume(ConfMan.getInt("music_volume")) != kRC_NOTIMPLEMENTED)
		_flags |= kCAPS_MUSICVOLUMESETTING;
	if (setSoundEffectVolume(ConfMan.getInt("sfx_volume")) != kRC_NOTIMPLEMENTED)
		_flags |= kCAPS_SFXVOLUMESETTING;
	if (setSpeechVolume(ConfMan.getInt("speech_volume")) != kRC_NOTIMPLEMENTED)
		_flags |= kCAPS_SPEECHVOLUMESETTING;
}

Port::~Port() {
	delete _midiInterface;
	delete _ph;
}

SoundType Port::getSoundType() const {
	return _ph->p_getType();
}

int Port::getCaps() const {
	return _flags;
}

void Port::write(uint32 address, uint8 value) {
	_ph->p_write(address, value);
}

uint8 Port::read(uint32 address) {
	return _ph->p_read(address);
}

int Port::runOpcode(uint8 opcode, ...) {
	va_list args;
	va_start(args, opcode);

	int res = _ph->p_opcode(opcode, args);

	va_end(args);
	return res;
}

MidiInterface *const Port::midiInterface() const {
	return _midiInterface;
}

Port::ReturnCode Port::setTimerCallbackReceiver(TimerCallbackReceiver *cb) {
	return (ReturnCode)_ph->p_setCbReceiver(cb);
}

Port::ReturnCode Port::setMusicVolume(int vol) {
	return (ReturnCode)_ph->p_setMusicVolume(vol);
}

Port::ReturnCode Port::setSoundEffectVolume(int vol) {
	return (ReturnCode)_ph->p_setSfxVolume(vol);
}

Port::ReturnCode Port::setSpeechVolume(int vol) {
	return (ReturnCode)_ph->p_setSpeechVolume(vol);
}

int Port::property(int prop, int value) {
	int res = (int)kRC_NOTIMPLEMENTED;
	switch (prop) {
	default:
		res = _ph->p_property(prop, value);
		break;
	}
		
	return res;
}

DeviceHandle detectDevice(Common::Platform platform, Common::List<SoundType> soundTypes) {

	//_devicehandles.push_back(0);
	return 0;
}

DeviceHandle detectDevice(Common::Platform platform, const SoundType *soundTypes) {
	//_devicehandles.push_back(0);
	return 0;
}

DeviceHandle detectDevice(Common::Platform platform, SoundType soundType) {
	//_devicehandles.push_back(0);
	return 0;
}

AudioDevice *validateHandle(DeviceHandle handle) {
	const PluginList p = MusicMan.getPlugins();

	if (p.begin() == p.end())
		error("Audio::validateHandle(): Audio plugins must be loaded prior to calling this function");

	for (PluginList::const_iterator m = p.begin(); m != p.end(); m++) {
		AudioDevices i = (*m)->get<AudioPluginObject>().getDevices();
		for (AudioDevices::iterator d = i.begin(); d != i.end(); d++) {
			if ((DeviceHandle)&(*d) == handle)
				return &(*d);
		}
	}

	return 0;
}

#define VALIDATEHANDLE(hdl, devPtr, retVal) \
	AudioDevice *##devPtr = validateHandle(hdl); \
	if (!##devPtr) \
		return retVal

SoundType getDeviceSoundType(DeviceHandle handle) {
	VALIDATEHANDLE(handle, dev, SND_VOID);
	return dev->getSoundType();
}

Port *createPort(DeviceHandle handle) {
	VALIDATEHANDLE(handle, dev, 0);
	return dev->createPort();
}

// device_intern.h functions

DeviceHandle getDeviceHandle(const Common::String &identifier) {
	const PluginList p = MusicMan.getPlugins();

	if (p.begin() == p.end())
		error("MidiDriver::getDeviceHandle: Music plugins must be loaded prior to calling this method");

	for (PluginList::const_iterator m = p.begin(); m != p.end(); m++) {
		AudioDevices i = (*m)->get<AudioPluginObject>().getDevices();
		for (AudioDevices::iterator d = i.begin(); d != i.end(); d++) {
			// The plugin id isn't unique, but it will match the plugin's first device. This is useful when selecting a "sound driver" from the command line.
			// TODO: CHECK THIS. Does the command line driver parameter still work/make sense?
			if (identifier.equals(d->getPluginId()) || identifier.equals(d->getCompleteId()) || identifier.equals(d->getCompleteName())) {
				DeviceHandle res = d->getHandle();
				//_devicehandles.push_back(res);
				return res;
			}
		}
	}

	return 0;
}

SoundTypes enumHardwareSoundTypes() {
	SoundTypes res;
	for (int i = 0; i < ARRAYSIZE(typeDesc); ++i) {
		if (typeDesc[i].supportsHardware)
			res.push_back(typeDesc[i].type);
	}
	return res;
}

const char *getSoundTypeDescription(SoundType soundType) {
	for (int i = 0; i < ARRAYSIZE(typeDesc); ++i) {
		if (typeDesc[i].type == soundType)
			return typeDesc[i].desc;
	}
	return 0;
}

Common::String &getDeviceDescription(DeviceHandle handle, DeviceDescriptionType type) {
	VALIDATEHANDLE(handle, dev, Common::String("auto"));

	switch (type) {
	case kPluginName:
		return dev->getPluginName();
	case kPluginId:
		return dev->getPluginId();
	case kDeviceName:
		return dev->getCompleteName();
	case kDeviceId:
		return dev->getCompleteId();
	default:
		break;
	}

	return Common::String("auto");
}

bool checkDevice(DeviceHandle handle) {
	VALIDATEHANDLE(handle, dev, false);
	return dev->check();
}

#undef VALIDATEHANDLE

} // end of namespace Audio
