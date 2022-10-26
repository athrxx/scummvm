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

#include <Windows.h>
#include <shlobj.h>
#include <tchar.h>

#include "backends/platform/sdl/win32/win32_wrapper.h"
#include "backends/vst/vst_detect.h"

namespace VST {

static const TCHAR *const defaultProgFilesSearchPaths[] = {
	_T("\\Vstplugins"),
	_T("\\Common Files\\VST2"),
	_T("\\Common Files\\VST3"),
	nullptr
};

static const int defaultProgFilesClasses[] = {
	CSIDL_PROGRAM_FILES,
#ifdef _WIN64
	CSIDL_PROGRAM_FILESX86,
#endif
	0
};

void tryLoad() {

}

PluginsSearchResult detectVSTPlugins() {
	PluginsSearchResult res;

	TCHAR pfolder[MAX_PATH];
	//Common::Array<Common::Path> searchPaths;
	//Common::Array<WindowsFilesystemNode> searchPaths;
	
	for (const int *cl = defaultProgFilesClasses; *cl; ++cl) {
		SHGetFolderPathFunc(NULL, *cl, NULL, SHGFP_TYPE_DEFAULT, pfolder);
		//TCHAR pfolder = Win32::tcharToString(str);
		for (const TCHAR *const *relPath = defaultProgFilesSearchPaths; *relPath; ++relPath) {
			//Common::String sfolder = pfolder + *relPath;
			_tcsncat(pfolder, *relPath, MAX_PATH);
			//searchPaths.push_back(WindowsFilesystemNode(sfolder, false));
			/*
			handle = FindFirstFile(charToTchar(searchPath), &desc);

			if (handle == INVALID_HANDLE_VALUE)
				return false;

			addFile(myList, mode, _path.c_str(), hidden, &desc);

			while (FindNextFile(handle, &desc))
				addFile(myList, mode, _path.c_str(), hidden, &desc);

			FindClose(handle);*/
			
		}
	}


	//Common::Array<Common::Path> searchPaths;
	//for (const char *const *path = defaultSearchPaths; *path; ++path)
	//	searchPaths.push_back(*path);
	//VST::PluginInfo info;

	//inf.name = "Roland Sound Canvas VA";
	//inf.type = MT_GM;
	res.push_back(PluginInfo("Roland Sound Canvas VA", "", 24, MT_GM));

	return res;
}

} // end of namespace VST

#endif
