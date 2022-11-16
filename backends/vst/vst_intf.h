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
#include "common/scummsys.h"
#include "common/memorypool.h"

class VSTInterface {
protected:
	VSTInterface(const Common::String &pluginName);
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
			// Unfortunately, the leading 0xF0 and trailing 0xF7 marker have to be stripped off for
			// ScummVM. Let's not talk about this "design decision". But we have to add them again...
			uint32 firstByteExt = 0;
			uint32 lastByteExt = 0;
			if (*data != 0xF0)
				firstByteExt++;
			if (data[dataSize - 1] != 0xF7)
				lastByteExt++;
			_dat += (firstByteExt + lastByteExt);
			uint8 *buf = new uint8[_dat];
			assert(buf);
			memcpy(buf + firstByteExt, data, dataSize);
			buf[0] = 0xF0;
			buf[_dat - 1] = 0xF7;
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

	const Common::String _pluginName;
	bool _hasEditor;

private:
	void loadSettings();
	void saveSettings();
	const Common::String getSaveFileName() const;
	virtual const char *getSaveFileExt() const = 0;

	virtual bool startPlugin() = 0;
	virtual void terminatePlugin() = 0;
	virtual uint32 getActiveSettings(uint8 **dest) const = 0;
	virtual void restoreSettings(const uint8 *data, uint32 dataSize) const = 0;
	virtual uint32 getActiveParameters(uint32 **dest) const = 0;
	virtual void restoreParameters(const uint32 *data, uint32 numPara) const = 0;

	const uint8 *_defaultSettings;
	uint32 _defaultSettingsSize;
	const uint32 *_defaultParameters;
	uint32 _numDefParameters;

public:
	static VSTInterface *create(MidiDriver::DeviceHandle dev);
};

#endif

#endif
