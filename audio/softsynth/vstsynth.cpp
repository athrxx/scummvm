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

#include "audio/mixer.h"
#include "audio/mpu401.h"
#include "audio/softsynth/vstsynth.h"
#include "backends/vst/vst_intf.h"

namespace VSTSynth {

class VSTMidiChannel final : public MidiChannel_MPU401 {
public:
	VSTMidiChannel(VSTMidiDriver *driver, int number) : MidiChannel_MPU401() {
		assert(driver);
		init(driver, number);
	}
	~VSTMidiChannel() override {}
};

VSTMidiDriver::VSTMidiDriver(DeviceHandle dev, Audio::Mixer *mixer) : MidiDriver_Emulated(mixer), _device(dev), _outputRate(mixer ? mixer->getOutputRate() : 0),
	_channels(nullptr), _numChannels(16), _intf(nullptr), _outputData(nullptr), _nullBuffer(nullptr), _outputDataSize(0), _spc(0), _spcr(0), _ssc(0), _sscr(0) {
	assert(mixer);

	_channels = new VSTMidiChannel*[_numChannels];
	assert(_channels);
	for (int i = 0; i < _numChannels; ++i)
		_channels[i] = new VSTMidiChannel(this, i);

	_spc = _outputRate / 250;
	_spcr = _outputRate % 250;
}

VSTMidiDriver::~VSTMidiDriver() {
	close();

	if (_channels) {
		for (int i = 0; i < _numChannels; ++i)
			delete _channels[i];
		delete[] _channels;
	}

	delete[] _outputData;
	delete[] _nullBuffer;
}

int VSTMidiDriver::open() {
	if (isOpen())
		return MERR_ALREADY_OPEN;

	if ((_intf = VSTInterface::create(_device)) == nullptr)
		return MERR_DEVICE_NOT_AVAILABLE;

	_intf->setSampleRate(_mixer->getOutputRate());
	_intf->setTempo(500000);

	if (_intf->open())
		return MERR_CANNOT_CONNECT;

	int res = MidiDriver_Emulated::open();

	_mixer->playStream(Audio::Mixer::kPlainSoundType, &_soundHandle, this, -1, Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO, true);

	_intf->runEditor();

	return res;
}

void VSTMidiDriver::close() {
	_mixer->stopHandle(_soundHandle);
	_isOpen = false;
	if (_intf) {
		_intf->close();
		delete _intf;
		_intf = nullptr;
	}
}

void VSTMidiDriver::send(uint32 msg) {
	if (!isOpen())
		return;
	Common::StackLock lock(_mixer->mutex());
	_intf->send(msg);
}

void VSTMidiDriver::sysEx(const byte *msg, uint16 length) {
	if (!isOpen())
		return;
	Common::StackLock lock(_mixer->mutex());
	_intf->sysex(msg, length);
}

void VSTMidiDriver::metaEvent(byte type, byte *data, uint16 len) {
	if (!isOpen() || type != 0x51 || len != 3)
		return;
	_intf->setTempo(data[0] << 16 | data[1] << 8 | data[2]);
}

MidiChannel *VSTMidiDriver::allocateChannel() {
	for (int i = 0; i < _numChannels; ++i) {
		if (i != 9 && _channels[i]->allocate())
			return _channels[i];
	}
	return nullptr;
}

MidiChannel *VSTMidiDriver::getPercussionChannel() {
	return _channels[9];
}

void VSTMidiDriver::generateSamples(int16 *buf, int len) {
	if (!isOpen())
		return;

	int bsize = isStereo() ? len << 1 : len;

	if (bsize > _outputDataSize) {
		delete[] _outputData;
		delete[] _nullBuffer;
		_outputData = new float[bsize]();
		_nullBuffer = new float[bsize]();
		_outputDataSize = bsize;
		_intf->setBlockSize(len);
	} else {
		memset(_outputData, 0, bsize * sizeof(float));
	}

	float *dest[2] = { _outputData, &_outputData[len] };
	float *dummySrc[2] = { _nullBuffer, &_nullBuffer[len] };

	_intf->generateSamples(dummySrc, dest, len, _spc - _ssc);

	uint32 rOffs = 0;
	if ((_sscr += _spcr) >= 250) {
		_sscr -= 250;
		rOffs = 1;
	}

	if ((_ssc += len) >= _spc + rOffs)
		_ssc = 0;

	const float *s1 = dest[0];
	const float *s2 = dest[1];

	if (isStereo()) {
		while (len--) {
			*buf++ = CLIP<int>((*s1 > 0 ? (int32)(*s1 * 32767.0f) : (int32)(*s1 * 32768.0f)), -32768, 32767);
			*buf++ = CLIP<int>((*s2 > 0 ? (int32)(*s2 * 32767.0f) : (int32)(*s2 * 32768.0f)), -32768, 32767);
			s1++;
			s2++;
		}
	} else {
		while (len--) {
			*buf++ = CLIP<int>((*s1 > 0 ? (int32)(*s1 * 32767.0f) : (int32)(*s1 * 32768.0f)), -32768, 32767);
			s1++;
		}
	}
}

}; // end of namespace VSTSynth

#endif
