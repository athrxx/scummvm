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

#ifndef BACKENDS_VSTDETECT_H
#define BACKENDS_VSTDETECT_H

#include "audio/mididrv.h"
#include "common/path.h"

namespace VST {

struct PluginInfo {
	PluginInfo(const char *pName, const char *pPath, int ver, MusicType tType) : name(pName), path(pPath), version(ver), type(tType) {}
	Common::String name;
	Common::Path path;
	int version;
	MusicType type;
};

typedef Common::Array<PluginInfo> PluginsSearchResult;

#if defined(WIN32) || defined(WINDOWS) /*|| defined(MACOSX)*/
PluginsSearchResult detectVSTPlugins();
#else
PluginsSearchResult detectVSTPlugins() { return PluginsSearchResult(); }
#endif

} // end of namespace VST

#endif

#endif
