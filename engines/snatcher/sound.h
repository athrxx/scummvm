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

#ifndef SNATCHER_SOUND_H
#define SNATCHER_SOUND_H

#include "common/platform.h"

namespace Snatcher {

class SoundDevice;

class SoundEngine {
public:
	SoundEngine(Common::Platform platform, int soundOptions);
	~SoundEngine();

	enum SoundFileType : int {
		kFMData = 0,
		kPCMData1,
		kPCMData2
	};

	void loadSoundFile(int type, const uint8 *data, uint32 dataSize);

	void musicPlay(int track);
	void musicStop();
	bool musicIsPlaying() const;
	uint32 musicGetTime() const;

	void fmSendCommand(int cmd, int arg);
	void pcmSendCommand(int cmd, int arg);
	void pcmSendCommand2(int cmd);

	void pause(bool toggle);

private:
	SoundDevice *_dev;
};

} // End of namespace Snatcher

#endif // SNATCHER_SOUND_H
