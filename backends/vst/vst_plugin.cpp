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


#if defined(USE_VST)

#include "audio/musicplugin.h"
#include "audio/softsynth/vstsynth.h"
#include "backends/vst/vst_detect.h"
#include "common/array.h"
#include "common/error.h"

class VSTSynthPlugin final : public MusicPluginObject {
public:
	const char *getName() const override {
		return "VST MIDI";
	}

	const char *getId() const override {
		return "vst";
	}

	MusicDevices getDevices() const override;
	bool checkDevice(MidiDriver::DeviceHandle) const override;
	Common::Error createInstance(MidiDriver **mididriver, MidiDriver::DeviceHandle dev) const override;

	VST::PluginsSearchResult &getSearchResult() const { return _pluginsSearchResult; }

private:
	mutable VST::PluginsSearchResult _pluginsSearchResult;
};

MusicDevices VSTSynthPlugin::getDevices() const {
	if (_pluginsSearchResult.empty())
		_pluginsSearchResult = VST::detectVSTPlugins();

	MusicDevices devices;

	if (!_pluginsSearchResult.empty()) {
		for (VST::PluginsSearchResult::const_iterator i = _pluginsSearchResult.begin(); i != _pluginsSearchResult.end(); ++i)
			devices.push_back(MusicDevice(this, i->name.c_str(), i->type));

	}

	return devices;
}

bool VSTSynthPlugin::checkDevice(MidiDriver::DeviceHandle) const {
	if (0) {
		warning("Failure");
		return false;
	}

	return true;
	}

Common::Error VSTSynthPlugin::createInstance(MidiDriver **mididriver, MidiDriver::DeviceHandle dev) const {
	*mididriver = new VSTSynth::VSTMidiDriver(dev, g_system->getMixer());
	return Common::kNoError;
}

const VST::PluginsSearchResult getSearchResult(const MusicPluginObject *plugin) {
	if (plugin) {
		VST::PluginsSearchResult &sr = static_cast<const VSTSynthPlugin*>(plugin)->getSearchResult();
		if (!sr.empty())
			return sr;
	}
	return VST::detectVSTPlugins();
}

//#if PLUGIN_ENABLED_DYNAMIC(VST)
//REGISTER_PLUGIN_DYNAMIC(VST, PLUGIN_TYPE_MUSIC, VSTSynthPlugin);
//#else
REGISTER_PLUGIN_STATIC(VST, PLUGIN_TYPE_MUSIC, VSTSynthPlugin);
//#endif

#endif
