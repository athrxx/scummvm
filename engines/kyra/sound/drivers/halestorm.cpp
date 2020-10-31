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

#include "kyra/kyra_v1.h"
#include "kyra/sound/drivers/halestorm.h"

namespace Kyra {

class HalestormInternal {
private:
	HalestormInternal(Audio::Mixer *mixer);
public:
	~HalestormInternal();

	static HalestormInternal *open(Audio::Mixer *mixer);
	static void close();

	bool init();

	void setMusicVolume(int volume);
	//void setSoundEffectVolume(int volume);

private:
	static HalestormInternal *_refInstance;
	static int _refCount;

	Audio::Mixer *_mixer;
	Audio::SoundHandle _soundHandle;
	bool _ready;
};

HalestormInternal *HalestormInternal::_refInstance = 0;
int HalestormInternal::_refCount = 0;

HalestormInternal::HalestormInternal(Audio::Mixer *mixer) : _mixer(mixer), _ready(false) {

}

HalestormInternal::~HalestormInternal() {
	// stopPaula();
	_mixer->stopHandle(_soundHandle);

	// Common::StackLock lock(_mutex);

	/// delete
}

HalestormInternal *HalestormInternal::open(Audio::Mixer *mixer) {
	_refCount++;

	if (_refCount == 1 && _refInstance == 0)
		_refInstance = new HalestormInternal(mixer);
	else if (_refCount < 2 || _refInstance == 0)
		error("HalestormInternal::open(): Internal instance management failure");

	return _refInstance;
}

void HalestormInternal::close() {
	if (!_refCount)
		return;

	_refCount--;

	if (!_refCount) {
		delete _refInstance;
		_refInstance = 0;
	}
}

bool HalestormInternal::init() {
	if (_ready)
		return true;

	////

	// startPaula();

	//_mixer->playStream(Audio::Mixer::kPlainSoundType,
	//	&_soundHandle, this, -1, Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO, true);

	_ready = true;

	return true;
}

void HalestormInternal::setMusicVolume(int volume) {

}

HalestormDriver::HalestormDriver(Audio::Mixer *mixer) {
	_hi = HalestormInternal::open(mixer);
}

HalestormDriver::~HalestormDriver() {
	HalestormInternal::close();
	_hi = 0;
}

bool HalestormDriver::init() {
	return _hi->init();
}

void HalestormDriver::setMusicVolume(int volume) {
	_hi->setMusicVolume(volume);
}

} // End of namespace Kyra

