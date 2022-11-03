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

#ifndef BACKENDS_VSTINTF_H
#define BACKENDS_VSTINTF_H

#include "audio/mididrv.h"
#include "common/memorypool.h"

class VSTInterface {
protected:
	VSTInterface();
public:
	virtual ~VSTInterface();

	int open();
	void close();

	virtual void setSampleRate(uint32 rate) = 0;
	virtual void setBlockSize(uint32 bsize) = 0;
	void send(uint32 msg);
	void sysex(const uint8 *msg, uint32 len);
	virtual void generateSamples(float **in, float **out, uint32 len) = 0;
	bool hasEditor() { return _hasEditor; }
	virtual void runEditor() = 0;

protected:
	void clearChain();

	struct EvtNode {
		EvtNode(EvtNode *chain, uint32 msg) : _next(chain), _syx(nullptr), _dat(msg) {}
		EvtNode(EvtNode *chain, const uint8 *data, uint32 dataSize) : _next(chain), _syx(nullptr), _dat(dataSize) {
			uint8 *buf = new uint8[dataSize];
			assert(buf);
			memcpy(buf, data, dataSize);
			_syx = buf;
		}
		~EvtNode() {
			delete[] _syx;
		}
		uint32 _dat;
		const uint8 *_syx;
		EvtNode *_next;
	};

	Common::ObjectPool<EvtNode> _eventsNodePool;
	EvtNode *_eventsChain;
	int _eventsCount;

	bool _hasEditor;

private:
	virtual bool startPlugin() = 0;
	virtual void terminatePlugin() = 0;
	virtual uint32 readSettings(uint8 **dest) = 0;
	virtual uint32 readParameters(uint32 **dest) = 0;

	uint8 *_defaultSettings;
	uint32 _settingsSize;
	uint32 *_defaultParameters;
	uint32 _numParameters;

public:
	static VSTInterface *create(MidiDriver::DeviceHandle dev);
};

#endif

#endif
