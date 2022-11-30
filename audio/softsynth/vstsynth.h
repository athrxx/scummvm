/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#if defined(USE_VST) && (defined(WIN32) || defined(WINDOWS) || defined(MACOSX))

#ifndef AUDIO_SOFTSYNTH_VSTSYNTH
#define AUDIO_SOFTSYNTH_VSTSYNTH

#include "audio/softsynth/emumidi.h"

class VSTInterface;

namespace VSTSynth {

class VSTMidiChannel;
class VSTMidiDriver final : public MidiDriver_Emulated {
public:
	VSTMidiDriver(DeviceHandle dev, Audio::Mixer *mixer);
	~VSTMidiDriver() override;

	int open() override;
	void close() override;

	void send(uint32 msg) override;
	void sysEx(const byte *msg, uint16 length) override;
	void metaEvent(byte type, byte *data, uint16 len) override;

	int getRate() const override { return _outputRate; }
	bool isStereo() const override { return true; }

	MidiChannel *allocateChannel() override;
	MidiChannel *getPercussionChannel() override;

private:
	void generateSamples(int16 *buf, int len) override;

	VSTMidiChannel **_channels;
	const int _numChannels;

	VSTInterface *_intf;
	const DeviceHandle _device;
	Audio::SoundHandle _soundHandle;

	float *_outputData;
	float *_nullBuffer;
	int _outputDataSize;

	const uint32 _outputRate;

	uint32 _spc, _spcr;
	uint32 _ssc, _sscr;
};

}; // end of namespace VSTSynth

#endif

#endif
