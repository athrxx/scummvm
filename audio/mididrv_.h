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

#ifndef AUDIO_MIDIDRV_H
#define AUDIO_MIDIDRV_H

#include "common/scummsys.h"
#include "common/str.h"
#include "common/timer.h"

/**
 * TODO: Document this, give it a better name.
 */
class MidiDriver_BASE {
public:
	virtual ~MidiDriver_BASE() { }

	/**
	 * Output a packed midi command to the midi stream.
	 * The 'lowest' byte (i.e. b & 0xFF) is the status
	 * code, then come (if used) the first and second
	 * opcode.
	 */
	virtual void send(uint32 b) = 0;

	/**
	 * Output a midi command to the midi stream. Convenience wrapper
	 * around the usual 'packed' send method.
	 *
	 * Do NOT use this for sysEx transmission; instead, use the sysEx()
	 * method below.
	 */
	void send(byte status, byte firstOp, byte secondOp) {
		send(status | ((uint32)firstOp << 8) | ((uint32)secondOp << 16));
	}

	/**
	 * Transmit a sysEx to the midi device.
	 *
	 * The given msg MUST NOT contain the usual SysEx frame, i.e.
	 * do NOT include the leading 0xF0 and the trailing 0xF7.
	 *
	 * Furthermore, the maximal supported length of a SysEx
	 * is 264 bytes. Passing longer buffers can lead to
	 * undefined behavior (most likely, a crash).
	 */
	virtual void sysEx(const byte *msg, uint16 length) { }

	// TODO: Document this.
	virtual void metaEvent(byte type, byte *data, uint16 length) { }
};

/*	enum DeviceStringType {
		kDriverName,
		kDriverId,
		kDeviceName,
		kDeviceId
	};

	static Common::String musicType2GUIO(uint32 musicType);

	
	static MidiDriver *createMidi(DeviceHandle handle);

	static DeviceHandle detectDevice(int flags);

		static DeviceHandle getDeviceHandle(const Common::String &identifier);

	static bool checkDevice(DeviceHandle handle);

	static MusicType getMusicType(DeviceHandle handle);

	static Common::String getDeviceString(DeviceHandle handle, DeviceStringType type);

private:
	// If detectDevice() detects MT32 and we have a preferred MT32 device
	// we use this to force getMusicType() to return MT_MT32 so that we don't
	// have to rely on the 'True Roland MT-32' config manager setting (since nobody
	// would possibly think about activating 'True Roland MT-32' when he has set
	// 'Music Driver' to '<default>')
	static bool _forceTypeMT32;

public:
	virtual ~MidiDriver() { }

	static const byte _mt32ToGm[128];
	static const byte _gmToMt32[128];

	/**
	 * Error codes returned by open.
	 * Can be converted to a string with getErrorName().
	 */
/*	enum {
		MERR_CANNOT_CONNECT = 1,
//		MERR_STREAMING_NOT_AVAILABLE = 2,
		MERR_DEVICE_NOT_AVAILABLE = 3,
		MERR_ALREADY_OPEN = 4
	};

	enum {
//		PROP_TIMEDIV = 1,
		PROP_OLD_ADLIB = 2,
		PROP_CHANNEL_MASK = 3,
		// HACK: Not so nice, but our SCUMM AdLib code is in audio/
		PROP_SCUMM_OPL3 = 4
	};
	*/
	/**
	 * Open the midi driver.
	 * @return 0 if successful, otherwise an error code.
	 */
	//virtual int open() = 0;

	/**
	 * Check whether the midi driver has already been opened.
	 */
	//virtual bool isOpen() const = 0;

	/** Close the midi driver. */
//	virtual void close() = 0;

	/** Get or set a property. */
	//virtual uint32 property(int prop, uint32 param) { return 0; }

	/** Retrieve a string representation of an error code. */
	//static const char *getErrorName(int error_code);

	// HIGH-LEVEL SEMANTIC METHODS
	/*virtual void setPitchBendRange(byte channel, uint range) {
		send(0xB0 | channel, 101, 0);
		send(0xB0 | channel, 100, 0);
		send(0xB0 | channel,   6, range);
		send(0xB0 | channel,  38, 0);
		send(0xB0 | channel, 101, 127);
		send(0xB0 | channel, 100, 127);
	}

	void sendMT32Reset();

	/**
	 * Send a General MIDI reset sysEx to the midi device.
	 */
//	void sendGMReset();

	//virtual void sysEx_customInstrument(byte channel, uint32 type, const byte *instr) { }

	// Timing functions - MidiDriver now operates timers
	//virtual void setTimerCallback(void *timer_param, Common::TimerManager::TimerProc timer_proc) = 0;

	/** The time in microseconds between invocations of the timer callback. */
	//virtual uint32 getBaseTempo() = 0;

	// Channel allocation functions
	//virtual MidiChannel *allocateChannel() = 0;
	//virtual MidiChannel *getPercussionChannel() = 0;
};
/*
class MidiChannel {
public:
	virtual ~MidiChannel() {}

	virtual MidiDriver *device() = 0;
	virtual byte getNumber() = 0;
	virtual void release() = 0;

	virtual void send(uint32 b) = 0; // 4-bit channel portion is ignored

	// Regular messages
	virtual void noteOff(byte note) = 0;
	virtual void noteOn(byte note, byte velocity) = 0;
	virtual void programChange(byte program) = 0;
	virtual void pitchBend(int16 bend) = 0; // -0x2000 to +0x1FFF

	// Control Change messages
	virtual void controlChange(byte control, byte value) = 0;
	virtual void modulationWheel(byte value) { controlChange(1, value); }
	virtual void volume(byte value) { controlChange(7, value); }
	virtual void panPosition(byte value) { controlChange(10, value); }
	virtual void pitchBendFactor(byte value) = 0;
	virtual void transpose(int8 value) {}
	virtual void detune(byte value) { controlChange(17, value); }
	virtual void priority(byte value) { }
	virtual void sustain(bool value) { controlChange(64, value ? 1 : 0); }
	virtual void effectLevel(byte value) { controlChange(91, value); }
	virtual void chorusLevel(byte value) { controlChange(93, value); }
	virtual void allNotesOff() { controlChange(123, 0); }

	// SysEx messages
	virtual void sysEx_customInstrument(uint32 type, const byte *instr) = 0;
};
*/
#endif
