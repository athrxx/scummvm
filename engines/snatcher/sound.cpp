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

#include "snatcher/sound_device.h"
#include "snatcher/sound.h"

namespace Snatcher {

SoundEngine::SoundEngine(Common::Platform platform, int soundOptions) : _dev(nullptr) {
	_dev = SoundDevice::create(platform, soundOptions);
	assert(_dev);
}

SoundEngine::~SoundEngine() {
	delete _dev;
}

void SoundEngine::loadSoundFile(int type, const uint8 *data, uint32 dataSize) {
	_dev->loadSoundFile(type, data, dataSize);
}

void SoundEngine::musicPlay(int track) {
	_dev->musicPlay(track);
}

void SoundEngine::musicStop() {
	_dev->musicStop();
}

bool SoundEngine::musicIsPlaying() const {
	return _dev->musicIsPlaying();
}

uint32 SoundEngine::musicGetTime() const {
	return _dev->musicGetTime();
}

void SoundEngine::fmSendCommand(int cmd, int arg) {
	if (cmd >= 241) {
		switch (cmd) {
		case 241:
			break;
		case 242:
			break;
		case 243:
			break;
		case 244:
			break;
		default:
			break;
		}
	} else if (cmd < 123) {
		if (cmd > 49 && cmd < 63) {
		} else {

		}
		
	}
	_dev->fmSendCommand(cmd);
}

void SoundEngine::pcmSendCommand(int cmd, int arg) {
	if (cmd >= 248) {
		switch (cmd) {
		case 252:
			break;
		case 253:
			break;
		case 255:
			break;
		default:
			break;
		}
	} else {
		_dev->pcmSendCommand(cmd, arg);
	}	
}

void SoundEngine::pcmSendCommand2(int cmd) {

}

void SoundEngine::pause(bool toggle) {
	_dev->pause(toggle);
}

SoundDevice *SoundDevice::create(Common::Platform platform, int soundOptions) {
	MidiDriver::DeviceHandle dev = MidiDriver::detectDevice(soundOptions);
	MusicType musicType = MidiDriver::getMusicType(dev);

	if (musicType == MT_INVALID)
		return nullptr;

	// TODO: We still need to create the appropriate sound device, since the game
	// heavily relies on audio sync for its animations. We'll just have to handle
	// it via volume settings.
	// if (musicType == MT_NULL)

	switch (platform) {
	case Common::kPlatformSegaCD:
		return createSegaSoundDevice();
	default:
		break;
	};

	return nullptr;
}

} // End of namespace Snatcher
