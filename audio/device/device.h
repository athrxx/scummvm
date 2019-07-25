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

#ifndef AUDIO_DEVICE_H
#define AUDIO_DEVICE_H

#include "common/platform.h"
#include "common/list.h"
#include "common/array.h"
#include "common/str.h"

enum SoundType {
	SND_VOID = -1,		// Invalid
	SND_AUTO = 0,		// Auto
	SND_NULL,			// Null
	SND_DIGITAL,		// Digital Audio
	SND_PCSPK,			// PC Speaker
	SND_PCJR,			// PCjr
	SND_CMS,			// CMS
	SND_ADLIB,			// AdLib, Sound Blaster etc.
	SND_OPL3,			// Sound Blaster Pro 2/16
	SND_DUALOPL,		// Sound Blaster Pro
	SND_GM,				// General MIDI
	SND_MT32,			// MT-32
	SND_GS,				// Roland GS
	SND_D110,			// Roland D-110
	SND_FB01,			// Yamaha FB01 / IBM Music Feature
	SND_CSM1,			// Casio CSM-1
	SND_C64,			// C64
	SND_AMIGA,			// Amiga
	SND_MAC,			// Apple Macintosh
	SND_APPLIIGS,		// Apple IIGS
	SND_TOWNS,			// FM-TOWNS
	SND_PC98_26,		// PC-9801-26
	SND_PC98_86,		// PC-9801-86
	SND_NES,			// Nintendo Entertainment System / Famicom
	SND_PCE				// Nec PC-Engine
};

namespace Audio {

class TimerCallbackReceiver {
public:
	virtual ~TimerCallbackReceiver() {}
	virtual void timerCallback(int) = 0;
};

class MidiInterface {
public:
	MidiInterface();
	~MidiInterface();

	// Send a midi message to the midi stream. Parameter 2 can be left empty if not needed.
	void sendMessage(uint8 byte, uint8 para1, uint8 para2 = 0);

	// Send a packed midi message to the midi stream. The 'lowest' byte(i.e.b & 0xFF) is
	// the status code, then come(if used) the first and second opcode.
	void sendMessage(uint32 msg);

	// This is the same as sendBytes, only that it will add F0 at the beginning and F7 at the end.
	void sendSysex(const uint8 *bytes, uint32 len);

	// TODO: Document this.
	void metaEvent(byte type, byte *data, uint32 length);
	
	// Send one or more bytes to the midi stream.
	virtual void sendByte(uint8 b) = 0;
	void sendBytes(uint32 numBytes, ...);
	void sendBytes(const uint8 *bytes, uint32 len);
};

class PortHandler;
class Port {
public:
	enum DeviceCapsFlags {
		kCAPS_MIDIINTERFACE			=		1 << 0,
		kCAPS_OPCODEINTERFACE		=		1 << 1,
		kCAPS_TIMERCALLBACK			=		1 << 2,
		kCAPS_MUSICVOLUMESETTING	=		1 << 3,
		kCAPS_SFXVOLUMESETTING		=		1 << 4,
		kCAPS_SPEECHVOLUMESETTING	=		1 << 5,
	};

	enum ReturnCode {
		kRC_NOTIMPLEMENTED			=		-1,
		kRC_SUCCESS					=		0,
		kRC_INVALIDARGUMENT			=		1,
		kRC_UNKNOWNERROR			=		2
	};

	enum Properties {
		kPROP_VOLUMECHANNELMASK		=		1,

		kPROP_SSGVOLUME				=		1001
	};

public:
	// The port handler will be deleted in the destructor
	Port(PortHandler *ph, bool provideMidiInterface);
	~Port();

	SoundType getSoundType() const;
	int getCaps() const;

	// The address should usually correspond to an i/o port but could also be an actual
	// memory location. See comment at the bottom of this file for createPort().
	// These methods are "guaranteed to work". There are no error codes.
	void write(uint32 address, uint8 value);
	uint8 read(uint32 address);

	// Allows sending commands via a driver-like opcode interface if the emulator or backend
	// driver is embedded into such low level driver code. Returns kRC_NOTIMPLEMENTED if such
	// an interface is not supported for the device (Presently, this method is implemented
	// specifically for the FM-Towns low level sound driver, since all games I've seen so far
	// use that).
	int runOpcode(uint8 opcode, ...);
	
	// Allows addressing Midi devices through Midi messages rather than writing to the (emulated)
	// ports if the emulator or backend driver supports it. Returns NULL if a Midi interface is
	// not available for the device.
	MidiInterface *const midiInterface() const;

	// Set a timer callback function if the emulator or backend driver supports that.
	// Return code depends on the successful installation of the callback. Only one
	// callback function is possible, since there is no need for more and this makes
	// it easier to remove the function. Pass a NULL argument to remove the callback.
	ReturnCode setTimerCallbackReceiver(TimerCallbackReceiver *cb);

	// For devices that handle the GMM (or corresponding engine) volume settings methods
	// on the lower level. Returns kRC_NOTIMPLEMENTED if the device does not support this.
	ReturnCode setMusicVolume(int vol);
	ReturnCode setSoundEffectVolume(int vol);
	ReturnCode setSpeechVolume(int vol);

	// This allows all kinds of individual settings depending on the emulator or backend
	// driver. Return code can be kRC_NOTIMPLEMENTED or whatever the property handler
	// wants to return. Some properties can be queried by passing a value of -1, but this
	// is device specific and thus not guaranteed.
	int property(int prop, int value);

private:
	PortHandler *_ph;
	MidiInterface *_midiInterface;

	int _flags;
};

typedef const void* const DeviceHandle;

DeviceHandle detectDevice(Common::Platform platform, Common::List<SoundType> soundTypes);

DeviceHandle detectDevice(Common::Platform platform, const SoundType *soundTypes);

DeviceHandle detectDevice(Common::Platform platform, SoundType soundType);

SoundType getDeviceSoundType(DeviceHandle handle);

// IMPORTANT: Despite its naming this is also supposed to work for emulated devices which
// don't get accessed via an i/o port but by direct memory access through addresses that
// are mapped into the normal address space (e. g. the Amiga with its 0xdff0ax addresses).
// In that case the Port::read/write commands should receive the appropriate memory address
// instead of a port address.
Port *createPort(DeviceHandle handle);

} // end of namespace Audio

#endif
