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

#include "common/config-manager.h"
#include "common/savefile.h"
#include "common/system.h"

#define VST_SAVE_VERSION	1

namespace VST {
#if defined(WIN32) || defined(WINDOWS)
VSTInterface *VSTInterface_WIN_create(const PluginInfo *target);
#elif defined(MACOSX)
VSTInterface *VSTInterface_MAC_create(const PluginInfo *target) { /*TODO*/ return nullptr; }
#endif
} // end of namespace VST

VSTInterface::VSTInterface(const Common::String &pluginName) : _pluginName(pluginName), _eventsChain(nullptr), _eventsCount(0), _defaultSettings(nullptr), _defaultSettingsSize(0), _defaultParameters(nullptr), _numDefParameters(0) {
}

VSTInterface::~VSTInterface() {
	clearChain();
}

int VSTInterface::open() {
	if (!startPlugin())
		return MidiDriver::MERR_CANNOT_CONNECT;

	uint8 *tmp1 = nullptr;
	_defaultSettingsSize = getActiveSettings(&tmp1);
	_defaultSettings = tmp1;

	if (_defaultSettings == nullptr || _defaultSettingsSize == 0) {
		uint32 *tmp2 = nullptr;
		_numDefParameters = getActiveParameters(&tmp2);
		_defaultParameters = tmp2;
	}

	loadSettings();

	return 0;
}

void VSTInterface::close() {
	saveSettings();

	terminatePlugin();

	delete[] _defaultSettings;
	_defaultSettings = nullptr;
	_defaultSettingsSize = 0;
	delete[] _defaultParameters;
	_defaultParameters = nullptr;
	_numDefParameters = 0;
}

void VSTInterface::setTempo(uint32 bpm) {
	// We just pass this on as a pseudo event that can be easily filtered out.
	send(bpm << 8 | 0xFF);
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

void VSTInterface::loadSettings() {
	Common::InSaveFile *s = g_system->getSavefileManager()->openForLoading(getSaveFileName());

	// This needn't be an error. If this a first-time run there won't be a config file.
	if (!s)
		return;

	bool failed = (s->readUint32BE() == MKTAG('S', 'V', 'M', ' ')) ? (s->readUint32BE() > VST_SAVE_VERSION): true;
	if (failed) {
		delete s;
		warning ("VSTInterface::loadSettings(): Could not process invalid settings file: '%s'", getSaveFileName().c_str());
		return;
	}

	if (_defaultSettings != nullptr && _defaultSettingsSize > 0) {
		uint32 curSettingsSize = s->readUint32BE();
		assert(curSettingsSize == _defaultSettingsSize);
		uint8 *curSettings = new uint8[curSettingsSize];
		s->read(curSettings, curSettingsSize);
		// We save only xor data against the default settings. With our
		// compression, this should make very small save files.
		const uint8 *src = _defaultSettings;
		uint8 *dst = curSettings;
		for (uint32 i = 0; i < curSettingsSize; ++i)
			*dst++ ^= *src++;
		restoreSettings(curSettings, curSettingsSize);
		delete[] curSettings;
	} else if (_defaultParameters && _numDefParameters > 0) {
		uint32 numParameters = s->readUint32BE();
		assert(numParameters == _numDefParameters);
		uint32 *curParameters = new uint32[numParameters];
		for (uint32 i = 0; i < numParameters; ++i)
			curParameters[i] = s->readUint32BE();
		// We save only xor data against the default settings. With our
		// compression, this should make very small save files.
		const uint32 *src2 = _defaultParameters;
		uint32 *dst2 = curParameters;
		for (uint32 i = 0; i < numParameters; ++i)
			*dst2++ ^= *src2++;
		restoreParameters(curParameters, numParameters);
		delete[] curParameters;
	}
	delete s;	
}

void VSTInterface::saveSettings() {
	Common::OutSaveFile *s = g_system->getSavefileManager()->openForSaving(getSaveFileName());
	if (!s) {
		warning("VSTInterface::saveSettings(): Unable to create config savefile");
		return;
	}

	s->writeUint32BE(MKTAG('S', 'V', 'M', ' '));
	s->writeUint32BE(VST_SAVE_VERSION);

	if (_defaultSettings != nullptr && _defaultSettingsSize > 0) {
		uint8 *curSettings = nullptr;
		uint32 curSettingsSize = getActiveSettings(&curSettings);
		assert(curSettingsSize == _defaultSettingsSize);
		// We save only xor data against the default settings. With our
		// compression, this should make very small save files.
		const uint8 *src = _defaultSettings;
		uint8 *dst = curSettings;
		for (uint32 i = 0; i < curSettingsSize; ++i)
			*dst++ ^= *src++;

		s->writeUint32BE(curSettingsSize);
		s->write(curSettings, curSettingsSize);
		delete[] curSettings;
	} else if (_defaultParameters && _numDefParameters > 0) {
		uint32 *curParameters = nullptr;
		uint32 numParameters = getActiveParameters(&curParameters);
		assert(numParameters == _numDefParameters);
		// We save only xor data against the default settings. With our
		// compression, this should make very small save files.
		const uint32 *src2 = _defaultParameters;
		uint32 *dst2 = curParameters;
		for (uint32 i = 0; i < numParameters; ++i)
			*dst2++ ^= *src2++;

		s->writeUint32BE(numParameters);
		for (uint32 i = 0; i < numParameters; ++i)
			s->writeUint32BE(curParameters[i]);
		delete[] curParameters;
	}
	s->finalize();
	delete s;
}

const Common::String VSTInterface::getSaveFileName() const {
	return ConfMan.getActiveDomainName() + '-' + _pluginName + '.' + getSaveFileExt();
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

#undef VST_SAVE_VERSION

#endif
