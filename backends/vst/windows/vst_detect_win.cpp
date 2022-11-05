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
#include "backends/vst/windows/vst_plug_win.h"
#include "common/config-manager.h"


#include "common/system.h"


namespace VST {

static const TCHAR *const defaultProgFilesSearchPaths[] = {
	_T("Vstplugins"),
	_T("Common Files\\VST2"),
	_T("Common Files\\VST3"),
	nullptr
};

static const int defaultProgFilesClasses[] = {
	CSIDL_PROGRAM_FILES,
#ifdef _WIN64
	// CSIDL_PROGRAM_FILESX86,
#endif
	0
};

const TCHAR *makeFullPath(const TCHAR *p1, const TCHAR *p2, bool appendDirSearchPattern) {
	TCHAR folderFullPath[MAX_PATH] = _T("");

	// trim '*' from the end of p1 if necessary
	size_t len1 = _tcsnlen(p1, ARRAYSIZE(folderFullPath) - 1);
	int trim = (p1[len1 - 1] == _T('*')) ? 1 : 0;
	_tcsncpy_s(folderFullPath, ARRAYSIZE(folderFullPath), p1, len1 - trim);

	// make sure sub strings are connected with a '\\'
	len1 = _tcsnlen(folderFullPath, ARRAYSIZE(folderFullPath) - 1);
	if ((folderFullPath[len1 - 1] != _T('\\')))
		_tcsncat_s(folderFullPath, ARRAYSIZE(folderFullPath), _T("\\"), _TRUNCATE);

	_tcsncat_s(folderFullPath, ARRAYSIZE(folderFullPath), p2, _TRUNCATE);
	if (appendDirSearchPattern)
		_tcsncat_s(folderFullPath, ARRAYSIZE(folderFullPath), _T("\\*"), _TRUNCATE);

	size_t len = _tcsnlen(folderFullPath, ARRAYSIZE(folderFullPath) - 1) + 1;
	TCHAR *res = new TCHAR[len];
	_tcsncpy_s(res, len, folderFullPath, len - 1);
	return res;
}

LPCVOID dllDetectCallback (LPVOID, int32 opcode, ...) {
	if (opcode == 1)
		return ConvertType<int>(2400).to<LPCVOID>();
	warning("detectVSTPlugins: Unexpected opcode '%d' received during detection.", opcode);
	return 0;
}

static PluginsSearchResult *_prefetchSearchResult = nullptr;
HANDLE _prefetchThread = nullptr;
DWORD _prefetchThreadId = 0;

PluginsSearchResult detectVSTPlugins() {
	PluginsSearchResult res;

	if (_prefetchThread && _prefetchThreadId != GetCurrentThreadId()) {
		uint32 wr = WaitForSingleObject(_prefetchThread, 10000);
		CloseHandle(_prefetchThread);
		_prefetchThread = nullptr;
		_prefetchThreadId = 0;
		if (wr != WAIT_OBJECT_0)
			warning("detectVSTPlugins(): %s", (wr == WAIT_TIMEOUT) ? "Prefetch thread timeout" : "Unknown prefetch thread failure");
	}

	if (_prefetchSearchResult)
		return *_prefetchSearchResult;

	TCHAR prgFolder[MAX_PATH] = _T("");

	// Add the usual VST plugin folders to the search.
	Common::Array<const TCHAR*> searchPaths;
	for (const int *cl = defaultProgFilesClasses; *cl; ++cl) {
		SHGetFolderPathFunc(NULL, *cl, NULL, SHGFP_TYPE_DEFAULT, prgFolder);
		for (const TCHAR *const *relPath = defaultProgFilesSearchPaths; *relPath; ++relPath)
			searchPaths.push_back(makeFullPath(prgFolder, *relPath, true));
	}

	// Also add ScummVM extra path...
	if (ConfMan.hasKey("extrapath")) {
		TCHAR *ppath = Win32::stringToTchar(ConfMan.get("extrapath"));
		searchPaths.push_back(makeFullPath(ppath, _T("*"), false));
		free(ppath);
	}

	WIN32_FIND_DATA data;
	memset(&data, 0, sizeof(data));

	do {
		Common::Array<const TCHAR*> searchPaths2;
		for (Common::Array<const TCHAR*>::const_iterator i = searchPaths.begin(); i != searchPaths.end(); ++i)
			searchPaths2.push_back(*i);
		searchPaths.clear();

		for (Common::Array<const TCHAR*>::const_iterator i = searchPaths2.begin(); i != searchPaths2.end(); ++i) {
			HANDLE handle = FindFirstFile(*i, &data);

			if (handle == INVALID_HANDLE_VALUE)
				continue;

			do {
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (data.cFileName[0] != _T('.'))
						// Add all sub dirs we encounter inside the specified paths
						searchPaths.push_back(makeFullPath(*i, data.cFileName, true));
				} else if (!_tcsnicmp(data.cFileName + _tcsnlen(data.cFileName, MAX_PATH) - 4, _T(".DLL"), 4)) {
					// Check if we found a v2 plugin...
					const TCHAR *testFile = makeFullPath(*i, data.cFileName, false);

					HMODULE testModule = nullptr;
					LPVOID sndWork = nullptr;

					if (!loadV2xPlugin(testFile, &dllDetectCallback, testModule, sndWork))
						continue;

					char name[64];
					if (READ_LE_UINT32(sndWork) == MKTAG('V', 's', 't', 'P')) {
						if ((plgProps(sndWork, 4) & 0x100) && (plgProps(sndWork, 3) < 3)) {
							// Query plugin name and version.
							V2XFunctor1 getName(sndWork, 45, 4);
							if (getName.isValid())
								getName(name);
							V2XFunctor1 getVersion(sndWork, 58, 0);
							int version = getVersion.isValid() ? getVersion() : 0;
							// Shut it down again. This seems to be required for some plugins,
							// despite not really having done anything yet.
							V2XFunctor1 quit(sndWork, 1, 0);
							if (quit.isValid())
								quit();

							Common::String pth = Win32::tcharToString(testFile);
							res.push_back(PluginInfo(name, pth, version, getPluginMusicType(name)));

						} else {
							warning("detectVSTPlugins(): Unsupported plugin: '%s'", Win32::tcharToString(testFile).c_str());
						}
					} else {
						warning("detectVSTPlugins(): Failed to initialize '%s'", Win32::tcharToString(testFile).c_str());
					}
	
					unloadV2xPlugin(testModule);
					delete[] testFile;

				} else if (!_tcsnicmp(data.cFileName + _tcsnlen(data.cFileName, MAX_PATH) - 5, _T(".VST3"), 5)) {
					// Otherwise, check for v3 plugin.
					const TCHAR *testFile = makeFullPath(*i, data.cFileName, false);

					/* TODO: */
					warning("detectVSTPlugins(): Skipping '%s' (VST 3.x not yet supported)", Win32::tcharToString(testFile).c_str());

					delete[] testFile;
				}

			} while (FindNextFile(handle, &data));

			FindClose(handle);
		}

		for (Common::Array<const TCHAR*>::const_iterator i = searchPaths2.begin(); i != searchPaths2.end(); ++i)
			delete[] *i;

		searchPaths2.clear();

	} while (!searchPaths.empty());

	return res;
}

// The detection takes very long. An we can't do much about it, since it's the init functions of the
// plugins that cause the delay (e.g. the Roland Sound Canvas VA needs over 2 seconds, ADLPlug and OPNPlug
// take around 700 - 800 msecs each). So, we make a prefetch detection in a thread...
DWORD prefetch_threadProc(LPVOID) {
	static PluginsSearchResult res = detectVSTPlugins();
	_prefetchSearchResult = &res;
	return 0;
}

void vstDetect_prefetch() {
	if (_prefetchSearchResult)
		return;
	_prefetchThread = CreateThread(NULL, NULL, &prefetch_threadProc, NULL, NULL, &_prefetchThreadId);
}

void vstDetect_releasePrefetchData() {
	if (!_prefetchSearchResult)
		return;

	if (_prefetchThread) {
		uint32 wr = WaitForSingleObject(_prefetchThread, 10000);
		CloseHandle(_prefetchThread);
		_prefetchThread = nullptr;
		_prefetchThreadId = 0;
		if (wr != WAIT_OBJECT_0)
			warning("detectVSTPlugins(): %s", (wr == WAIT_TIMEOUT) ? "Prefetch thread timeout" : "Unknown prefetch thread failure");
	}

	_prefetchSearchResult->clear();
	_prefetchSearchResult = nullptr;
}

} // end of namespace VST

#endif
