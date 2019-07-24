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

#ifndef SCUMM_IMUSE_ADL_H
#define SCUMM_IMUSE_ADL_H

#include "audio/fmopl.h"
#include "audio/mididrv.h"


 ////////////////////////////////////////
 //
 // AdLib MIDI driver
 //
 ////////////////////////////////////////

namespace Scumm {

#ifdef DEBUG_ADLIB
static int g_tick;
#endif

// Only include OPL3 when we actually have an AdLib emulator builtin, which
// supports OPL3.
#ifndef DISABLE_DOSBOX_OPL
#define ENABLE_OPL3
#endif

class MidiDriver_ADLIB;
struct AdLibVoice;

// We use packing for the following two structs, because the code
// does simply copy them over from byte streams, without any
// serialization. Check AdLibPart::sysEx_customInstrument for an
// example of this.
//
// It might be very well possible, that none of the compilers we support
// add any padding bytes at all, since the structs contain only variables
// of the type 'byte'. But better safe than sorry.
#include "common/pack-start.h"
struct InstrumentExtra {
	byte a, b, c, d, e, f, g, h;
} PACKED_STRUCT;

struct AdLibInstrument {
	byte modCharacteristic;
	byte modScalingOutputLevel;
	byte modAttackDecay;
	byte modSustainRelease;
	byte modWaveformSelect;
	byte carCharacteristic;
	byte carScalingOutputLevel;
	byte carAttackDecay;
	byte carSustainRelease;
	byte carWaveformSelect;
	byte feedback;
	byte flagsA;
	InstrumentExtra extraA;
	byte flagsB;
	InstrumentExtra extraB;
	byte duration;
} PACKED_STRUCT;
#include "common/pack-end.h"

class AdLibPart : public MidiChannel {
	friend class MidiDriver_ADLIB;

protected:
//	AdLibPart *_prev, *_next;
	AdLibVoice *_voice;
	int16 _pitchBend;
	byte _pitchBendFactor;
	//int8 _transposeEff;
	byte _volEff;
	int8 _detuneEff;
	byte _modWheel;
	bool _pedal;
	byte _program;
	byte _priEff;
	byte _pan;
	AdLibInstrument _partInstr;
#ifdef ENABLE_OPL3
	AdLibInstrument _partInstrSecondary;
#endif

protected:
	MidiDriver_ADLIB *_owner;
	bool _allocated;
	byte _channel;

	void init(MidiDriver_ADLIB *owner, byte channel);
	void allocate() { _allocated = true; }

public:
	AdLibPart() {
		_voice = 0;
		_pitchBend = 0;
		_pitchBendFactor = 2;
		//_transposeEff = 0;
		_volEff = 0;
		_detuneEff = 0;
		_modWheel = 0;
		_pedal = 0;
		_program = 0;
		_priEff = 0;
		_pan = 64;

		_owner = 0;
		_allocated = false;
		_channel = 0;

		memset(&_partInstr, 0, sizeof(_partInstr));
#ifdef ENABLE_OPL3
		memset(&_partInstrSecondary, 0, sizeof(_partInstrSecondary));
#endif
	}

	MidiDriver *device();
	byte getNumber() { return _channel; }
	void release() { _allocated = false; }

	void send(uint32 b);

	// Regular messages
	void noteOff(byte note);
	void noteOn(byte note, byte velocity);
	void programChange(byte program);
	void pitchBend(int16 bend);

	// Control Change messages
	void controlChange(byte control, byte value);
	void modulationWheel(byte value);
	void volume(byte value);
	void panPosition(byte value);
	void pitchBendFactor(byte value);
	void detune(byte value);
	void priority(byte value);
	void sustain(bool value);
	void effectLevel(byte value) { return; } // Not supported
	void chorusLevel(byte value) { return; } // Not supported
	void allNotesOff();

	// SysEx messages
	void sysEx_customInstrument(uint32 type, const byte *instr);
};

// FYI (Jamieson630)
// It is assumed that any invocation to AdLibPercussionChannel
// will be done through the MidiChannel base class as opposed to the
// AdLibPart base class. If this were NOT the case, all the functions
// listed below would need to be virtual in AdLibPart as well as MidiChannel.
class AdLibPercussionChannel : public AdLibPart {
	friend class MidiDriver_ADLIB;

protected:
	void init(MidiDriver_ADLIB *owner, byte channel);

public:
	~AdLibPercussionChannel();

	void noteOff(byte note);
	void noteOn(byte note, byte velocity);
	void programChange(byte program) { }

	// Control Change messages
	void modulationWheel(byte value) { }
	void pitchBendFactor(byte value) { }
	void detune(byte value) { }
	void priority(byte value) { }
	void sustain(bool value) { }

	// SysEx messages
	void sysEx_customInstrument(uint32 type, const byte *instr);

private:
	byte _notes[256];
	AdLibInstrument *_customInstruments[256];
};

struct Struct10 {
	byte active;
	int16 curVal;
	int16 count;
	uint16 maxValue;
	int16 startValue;
	byte loop;
	byte tableA[4];
	byte tableB[4];
	int8 unk3;
	int8 modWheel;
	int8 modWheelLast;
	uint16 speedLoMax;
	uint16 numSteps;
	int16 speedHi;
	int8 direction;
	uint16 speedLo;
	uint16 speedLoCounter;
};

struct Struct11 {
	int16 modifyVal;
	byte param, flag0x40, flag0x10;
	Struct10 *s10;
};

struct AdLibVoice {
	AdLibPart *_part;
	AdLibVoice *_next, *_prev;
	byte _waitForPedal;
	byte _note;
	byte _channel;
	byte _twoChan;
	byte _vol1, _vol2;
	int16 _duration;

	Struct10 _s10a;
	Struct11 _s11a;
	Struct10 _s10b;
	Struct11 _s11b;

#ifdef ENABLE_OPL3
	byte _secTwoChan;
	byte _secVol1, _secVol2;
#endif

	AdLibVoice() { memset(this, 0, sizeof(AdLibVoice)); }
};

class MidiDriver_ADLIB : public MidiDriver {
	friend class AdLibPart;
	friend class AdLibPercussionChannel;

public:
	MidiDriver_ADLIB();

	int open();
	void close();
	void send(uint32 b);
	void send(byte channel, uint32 b); // Supports higher than channel 15
	uint32 property(int prop, uint32 param);
	bool isOpen() const { return _isOpen; }
	uint32 getBaseTempo() { return 1000000 / OPL::OPL::kDefaultCallbackFrequency; }

	void setPitchBendRange(byte channel, uint range);
	void sysEx_customInstrument(byte channel, uint32 type, const byte *instr);

	MidiChannel *allocateChannel();
	MidiChannel *getPercussionChannel() { return &_percussion; } // Percussion partially supported

	virtual void setTimerCallback(void *timerParam, Common::TimerManager::TimerProc timerProc);

private:
	bool _scummSmallHeader; // FIXME: This flag controls a special mode for SCUMM V3 games
#ifdef ENABLE_OPL3
	bool _opl3Mode;
#endif

	OPL::OPL *_opl;
	byte *_regCache;
#ifdef ENABLE_OPL3
	byte *_regCacheSecondary;
#endif

	Common::TimerManager::TimerProc _adlibTimerProc;
	void *_adlibTimerParam;

	int _timerCounter;

	uint16 _channelTable2[9];
	int _voiceIndex;
	int _timerIncrease;
	int _timerThreshold;
	uint16 _curNotTable[9];
	AdLibVoice _voices[9];
	AdLibPart _parts[32];
	AdLibPercussionChannel _percussion;

	bool _isOpen;

	void onTimer();
	void partKeyOn(AdLibPart *part, const AdLibInstrument *instr, byte note, byte velocity, const AdLibInstrument *second, byte pan);
	void partKeyOff(AdLibPart *part, byte note);

	void adlibKeyOff(int chan);
	void adlibNoteOn(int chan, byte note, int mod);
	void adlibNoteOnEx(int chan, byte note, int mod);
	int adlibGetRegValueParam(int chan, byte data);
	void adlibSetupChannel(int chan, const AdLibInstrument *instr, byte vol1, byte vol2);
#ifdef ENABLE_OPL3
	void adlibSetupChannelSecondary(int chan, const AdLibInstrument *instr, byte vol1, byte vol2, byte pan);
#endif
	byte adlibGetRegValue(byte reg) {
		return _regCache[reg];
	}
#ifdef ENABLE_OPL3
	byte adlibGetRegValueSecondary(byte reg) {
		return _regCacheSecondary[reg];
	}
#endif
	void adlibSetParam(int channel, byte param, int value, bool primary = true);
	void adlibKeyOnOff(int channel);
	void adlibWrite(byte reg, byte value);
#ifdef ENABLE_OPL3
	void adlibWriteSecondary(byte reg, byte value);
#endif
	void adlibPlayNote(int channel, int note);

	AdLibVoice *allocateVoice(byte pri);

	void mcOff(AdLibVoice *voice);

	static void linkMc(AdLibPart *part, AdLibVoice *voice);
	void mcIncStuff(AdLibVoice *voice, Struct10 *s10, Struct11 *s11);
	void mcInitStuff(AdLibVoice *voice, Struct10 *s10, Struct11 *s11, byte flags,
					   const InstrumentExtra *ie);

	void struct10Init(Struct10 *s10, const InstrumentExtra *ie);
	static byte struct10OnTimer(Struct10 *s10, Struct11 *s11);
	static void struct10Setup(Struct10 *s10);
	static int randomNr(int a);
	void mcKeyOn(AdLibVoice *voice, const AdLibInstrument *instr, byte note, byte velocity, const AdLibInstrument *second, byte pan);
};

} // End of namespace Scumm

#endif
