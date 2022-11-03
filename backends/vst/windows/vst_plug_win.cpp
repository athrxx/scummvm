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

 
#if defined(USE_VST) && (defined(WIN32) || defined(WINDOWS))

#include "backends/vst/windows/vst_plug_win.h"

namespace VST {

bool loadV2xPlugin(const TCHAR *dllPath, hostCBProc proc, HMODULE &destModule, LPVOID &destSndWork) {
	destSndWork = nullptr;

	destModule = LoadLibrary(dllPath);
	if (!destModule)
		return false;

	typedef LPVOID (*initProc)(hostCBProc);

	initProc vstPlugEntry = (initProc)(FARPROC)GetProcAddress(destModule, "VSTPluginMain");
	// At least for the Roland Sound Canvas VA one of these functions seems to be a wrapper for
	// the other. They both work exactly the same. Since I am not sure if one of these is definitely
	// guaranteed I check for both.
	if (!vstPlugEntry)
		vstPlugEntry = (initProc)(FARPROC)GetProcAddress(destModule, "main");

	if (vstPlugEntry)
		destSndWork = vstPlugEntry(*proc);

	return (destSndWork != nullptr);
}

void unloadV2xPlugin(HMODULE module) {
	FreeLibrary(module);
}

} // end of namespace VST

#endif
