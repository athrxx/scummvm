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

#include "kyra/sound/sound_intern.h"
#include "kyra/sound/drivers/halestorm.h"
//#include "kyra/resource/resource.h"
#include "common/macresman.h"

#include "audio/mixer.h"

namespace Kyra {

SoundMac::SoundMac(KyraEngine_v1 *vm, Audio::Mixer *mixer) : Sound(vm, mixer), _driver(0), _macRes(0) {
}

SoundMac::~SoundMac() {
	delete _driver;
	delete _macRes;
}

Sound::kType SoundMac::getMusicType() const {
	return kMac;
}

bool SoundMac::init() {
	_macRes = new Common::MacResManager();
	_driver = new HalestormDriver(_mixer);
	return _driver && _driver->init();
}

bool SoundMac::hasSoundFile(uint file) const {
	// if (file < 3)
	//	return true;
	return false;
}

void SoundMac::loadSoundFile(uint file) {
	
}

void SoundMac::playTrack(uint8 track) {

}

void SoundMac::haltTrack() {

}

void SoundMac::playSoundEffect(uint16 track, uint8 volume) {

}

bool SoundMac::isPlaying() const {
	return false;
}
 
void SoundMac::beginFadeOut() {

}

void SoundMac::updateVolumeSettings() {

}

} // End of namespace Kyra
