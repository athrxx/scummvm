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

#include "audio/musicplugin.h"
#include "backends/vst/vst_detect.h"
#include "backends/vst/vst_intf.h"


namespace VST {
#if defined(WIN32) || defined(WINDOWS)
VSTInterface *VSTInterface_WIN_create(const PluginInfo *target);
#elif defined(MACOSX)
VSTInterface *VSTInterface_MAC_create(const PluginInfo *target) { /*TODO*/ return nullptr; }
#endif
} // end of namespace VST


VSTInterface::VSTInterface() : _eventsChain(nullptr), _eventsCount(0), _defaultSettings(nullptr), _settingsSize(0), _defaultParameters(nullptr), _numParameters(0) {
}

VSTInterface::~VSTInterface() {
	clearChain();
}

int VSTInterface::open() {
	if (!startPlugin())
		return MidiDriver::MERR_CANNOT_CONNECT;

	_settingsSize = readSettings(&_defaultSettings);
	_numParameters = readParameters(&_defaultParameters);

	return 0;
}

void VSTInterface::close() {
	terminatePlugin();
	_defaultSettings = nullptr;
	_settingsSize = 0;
	delete[] _defaultParameters;
	_defaultParameters = nullptr;
	_numParameters = 0;
}

void VSTInterface::send(uint32 msg) {
	_eventsChain = new (_eventsNodePool) EvtNode(_eventsChain, msg);
	++_eventsCount;
}

void VSTInterface::sysex(const uint8 *msg, uint32 len) {
	_eventsChain = new (_eventsNodePool) EvtNode(_eventsChain, msg, len);
	++_eventsCount;
}

void VSTInterface::clearChain() {
	while (_eventsChain) {
		EvtNode *e = _eventsChain;
		_eventsChain = _eventsChain->_next;
		_eventsNodePool.deleteChunk(e);
	}
	_eventsCount = 0;
}

const VST::PluginsSearchResult getSearchResult(const MusicPluginObject *plugin);

VSTInterface *VSTInterface::create(MidiDriver::DeviceHandle dev) {
	VSTInterface *res = nullptr;
	VST::PluginsSearchResult sr;

	// First we fetch the list with the detected VST plugins (either from the plugin
	const PluginList p = MusicMan.getPlugins();
	for (PluginList::const_iterator m = p.begin(); m != p.end(); m++) {
		const MusicPluginObject &musicPlugin = (*m)->get<MusicPluginObject>();
		if (MidiDriver::getDeviceString(dev, MidiDriver::kDriverId).equals(musicPlugin.getId())) {
			if (!sr.empty()) // I don't see how this could happen, but let's check anyway...
				warning("VSTInterface::create(): Encountered multiple instances of the ScummVM VST plugin");
			sr = getSearchResult(&musicPlugin);
		}
	}

	const VST::PluginInfo *target = nullptr;
	Common::String devName = MidiDriver::getDeviceString(dev, MidiDriver::kDeviceShortName);
	for (VST::PluginsSearchResult::const_iterator i = sr.begin(); target == nullptr && i != sr.end(); ++ i) {
		if (devName.equals(i->name))
			target = i;
	}

	if (!target) {
		error("VSTInterface::create(): Unable to connect to VST plugin '%s'", devName.c_str());
		return res;
	}

#if defined(WIN32) || defined(WINDOWS)
	res = VST::VSTInterface_WIN_create(target);
#elif defined(MACOSX)
	res = VST::VSTInterface_MAC_create(target);
#endif

	return res;
}

#endif
