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
#include <tchar.h>

#include "backends/platform/sdl/win32/win32_wrapper.h"
#include "backends/vst/vst_detect.h"
#include "backends/vst/vst_intf.h"
#include "backends/vst/windows/vst_plug_win.h"

#include "common/func.h"
#include "common/scummsys.h"

namespace VST {

#define pluginCall(a) if (_op_##a && _op_##a->isValid()) (*_op_##a)
#define pluginCallRes(a) !(_op_##a && _op_##a->isValid()) ? 0 : (*_op_##a)
#define pluginCallArgI32(a, val) pluginCall(a)(ConvertType<uint32>(val).to<LPVOID>())

class VSTInterface_2X_WIN final : public VSTInterface {
public:
	VSTInterface_2X_WIN(const Common::String &pluginPath);
	~VSTInterface_2X_WIN() override;

	void setSampleRate(uint32 rate) override;
	void setBlockSize(uint32 bsize) override;
	void generateSamples(float **in, float **out, uint32 len) override;
	void runEditor() override;

private:
	bool startPlugin() override;
	void terminatePlugin() override;
	void midiSendAll();
	void resume();
	uint32 readSettings(uint8 **dest) override;
	uint32 readParameters(uint32 **dest) override;

	HMODULE _plugin;
	LPVOID _sndWork;

	const uint8 *_msgBuffer;

	uint32 _rate;
	uint32 _bsize;

	const Common::String _pluginPath;;
	bool _ready;

private:
	V2XFunctor1 *_op_close;
	V2XFunctor1 *_op_stop;
	V2XFunctor1 *_op_setRate;
	V2XFunctor1 *_op_setBSize;
	V2XFunctor1 *_op_sendMsg;
	V2XFunctor1 *_op_setOnOff;
	V2XFunctor2 *_op_genSamples;
	//V2XFunctor1 *_op_getParaDsc;
	V2XFunctor1 *_op_readPara;
	V2XFunctor1 *_op_writePara;
	V2XFunctor1 *_op_readSettings;
	V2XFunctor1 *_op_writeSettings;
	V2XFunctor1 *_op_runEditor;

	static LPCVOID dllCallback(LPVOID, int32 opcode, ...);
};

static const char *_pluginFolder = nullptr;

VSTInterface_2X_WIN::VSTInterface_2X_WIN(const Common::String &pluginPath) : VSTInterface(), _pluginPath(pluginPath), _plugin(nullptr), _sndWork(nullptr), _ready(false),
	_bsize(0), _op_close(nullptr), _op_stop(nullptr), _op_setRate(nullptr), _op_setBSize(nullptr), _op_sendMsg(nullptr), _op_setOnOff(nullptr), _op_genSamples(nullptr),
	/*_op_getParaDsc, */_op_readPara(nullptr), _op_writePara(nullptr), _op_readSettings(nullptr), _op_writeSettings(nullptr), _op_runEditor(nullptr), _msgBuffer(nullptr),
	_rate(0) {

	Common::String folder = _pluginPath.substr(0, _pluginPath.findLastOf('\\'));
	char *folderStr = new char[folder.size() + 1];
	assert(folderStr);
	Common::strlcpy(folderStr, folder.c_str(), folder.size() + 1);
	_pluginFolder = folderStr;
}

VSTInterface_2X_WIN::~VSTInterface_2X_WIN() {
	terminatePlugin();
	delete[] _pluginFolder;
	_pluginFolder = nullptr;
	delete[] _msgBuffer;
}

void VSTInterface_2X_WIN::setSampleRate(uint32 rate) {
	_rate = rate;
	if (!_ready)
		return;
	pluginCall(setRate)((float)rate);
	resume();	
}

void VSTInterface_2X_WIN::setBlockSize(uint32 bsize) {
	_bsize = bsize;
	if (!_ready)
		return;
	pluginCallArgI32(setBSize, bsize);
	resume();
}

void VSTInterface_2X_WIN::generateSamples(float **in, float **out, uint32 len) {
	if (!_ready)
		return;
	midiSendAll();
	pluginCall(genSamples)(in, out, len);
	clearChain();
}

LRESULT eproc(HWND hWnd, UINT mg, WPARAM w, LPARAM l) {
	MSG m;

/*	while (GetMessage(&m, NULL, 0, 0) > 0) {
		TranslateMessage(&m);
		DispatchMessage(&m);
	}
	*/
	return 0;
}

void VSTInterface_2X_WIN::runEditor() {
	TCHAR wname[] = _T("Sound Plugin Config");
	WNDCLASS wcl = { 0, &eproc, 0, 0, GetModuleHandle(NULL), nullptr, nullptr, nullptr, _T(""), wname };
	RegisterClass(&wcl);
	HWND hWnd =	CreateWindow(wname, wname,  WS_OVERLAPPEDWINDOW, 100, 100, 1000, 1000, NULL, NULL, GetModuleHandle(NULL), NULL);
	ShowWindow(hWnd, WM_SHOWWINDOW);

	pluginCall(runEditor)(hWnd);
}

bool VSTInterface_2X_WIN::startPlugin() {
	TCHAR *ppath = Win32::stringToTchar(_pluginPath);
	bool res = loadV2xPlugin(ppath, &dllCallback, _plugin, _sndWork);
	free(ppath);

	if (!res)
		return false;

	// We don't need to repeat the validity tests here that were already done during the
	// detection process, since we will never arrive here with a plugin that failed detection.

	_hasEditor = plgProps(_sndWork, 4) & 1;

	// Make the plugin access functions that we need.
	_op_close = new V2XFunctor1(_sndWork, 1, 0);
	assert(_op_close);
	_op_stop = new V2XFunctor1(_sndWork, 72, 0);
	assert(_op_stop);
	_op_setRate = new V2XFunctor1(_sndWork, 10, 8);
	assert(_op_setRate);
	_op_setBSize = new V2XFunctor1(_sndWork, 11, 2);
	assert(_op_setBSize);
	_op_sendMsg = new V2XFunctor1(_sndWork, 25, 4);
	assert(_op_sendMsg);
	_op_setOnOff = new V2XFunctor1(_sndWork, 12, 2);
	assert(_op_setOnOff);
	//_op_getParaDsc = new V2XFunctor1(_sndWork, 8, 5);
	//assert(_op_getParaDsc);
	_op_readPara = new V2XFunctor1(_sndWork, 14, 4);
	assert(_op_readPara);
	_op_writePara = new V2XFunctor1(_sndWork, 14, 4);
	assert(_op_writePara);
	_op_readSettings = new V2XFunctor1(_sndWork, 23, 4);
	assert(_op_readSettings);
	_op_writeSettings = new V2XFunctor1(_sndWork, 24, 5);
	assert(_op_writeSettings);
	_op_runEditor = new V2XFunctor1(_sndWork, 14, 4);
	assert(_op_runEditor);

	_op_genSamples = new V2XFunctor2(_sndWork, 10, 10);
	assert(_op_genSamples);
	if (!_op_genSamples->isValid()) {
		delete _op_genSamples;
		// If the normal samples processing routine doesn't exist, we look for the older (deprecated) one.
		_op_genSamples = new V2XFunctor2(_sndWork, 2, 0);
		assert(_op_genSamples);
	}

	/*char dsc[64] = "";
	for (int i = 0; i < _numPara; ++i) {
		pluginCall(getParaDsc)(i, dsc);
		if (dsc[0] == '\0')
			break;
		warning("Parameter No. %d: '%s'", i, dsc);
	}*/

	// We need these only here. No need to keep them afterwards...
	V2XFunctor1 op_open(_sndWork, 0, 0);
	if (op_open.isValid())
		op_open();

	V2XFunctor1 op_start(_sndWork, 71, 0);
	if (op_start.isValid())
		op_start();

	_ready = true;

	if (_rate)
		setSampleRate(_rate);
	if (_bsize)
		setBlockSize(_bsize);

	return res;
}

void VSTInterface_2X_WIN::terminatePlugin() {
	_ready = false;

	if (!_plugin)
		return;

	pluginCall(stop)();
	pluginCall(close)();
	_sndWork = nullptr;

	unloadV2xPlugin(_plugin);
	_plugin = nullptr;
	_hasEditor = false;

	delete _op_close;
	_op_close = nullptr;
	delete _op_stop;
	_op_stop = nullptr;
	delete _op_setRate;
	_op_setRate = nullptr;
	delete _op_setBSize;
	_op_setBSize = nullptr;
	delete _op_sendMsg;
	_op_sendMsg = nullptr;
	delete _op_setOnOff;
	_op_setOnOff = nullptr;
	//delete _op_getParaDsc;
	//_op_getParaDsc = nullptr;
	delete _op_readPara;
	_op_readPara = nullptr;
	delete _op_writePara;
	_op_writePara = nullptr;
	delete _op_readSettings;
	_op_readSettings = nullptr;
	delete _op_writeSettings;
	_op_writeSettings = nullptr;
	delete _op_runEditor;
	_op_runEditor = nullptr;
	delete _op_genSamples;
	_op_genSamples = nullptr;
}

void VSTInterface_2X_WIN::midiSendAll() {
	if (!_ready || !_eventsCount)
		return;

	delete[] _msgBuffer;
	uint32 buffSize = _eventsCount * (48 + sizeof(LPVOID)) + 3 * sizeof(LPVOID);
	uint8 *buff = new uint8[buffSize];
	memset(buff, 0, buffSize);
	const uint8 *package = &buff[_eventsCount * 48];
	const uint8 **table = &((const uint8**)package)[_eventsCount + 1];
	uint8 *pos = buff;

	for (EvtNode *e = _eventsChain; e; e = e->_next) {
		if (e->_syx) {
			pos[0] = 6;
			pos[4] = (4 + sizeof(LPVOID)) << 2;
			*(const uint8**)&pos[(8 + sizeof(LPVOID)) << 1] = e->_syx;
			WRITE_UINT32(&pos[16], e->_dat);
		} else {
			pos[0] = pos[12] = 1;
			pos[4] = 0x20;
			WRITE_LE_UINT32(&pos[24], e->_dat);
		}
		// Reverse order in the table, since _eventsChain is a fifo chain
		*table-- = pos;
		pos += 48;
	}

	WRITE_UINT32(pos, _eventsCount);

	pluginCall(sendMsg)(package);
	_msgBuffer = buff;
}

void VSTInterface_2X_WIN::resume() {
	pluginCallArgI32(setOnOff, 1);
}

uint32 VSTInterface_2X_WIN::readSettings(uint8 **dest) {
	*dest = nullptr;
	if (!_ready)
		return 0;

	uint32 size = 0;
	if (plgProps(_sndWork, 4) & 0x20)
		size = pluginCallRes(readSettings)(dest);

	return size;
}

uint32 VSTInterface_2X_WIN::readParameters(uint32 **dest) {
	*dest = nullptr;

	if (!_ready)
		return 0;

	uint32 num = plgProps(_sndWork, 1);
	if (!num)
		return 0;

	*dest = new uint32[num];

	for (uint32 i = 0; i < num; ++i)
	{}

	return num;
}

LPCVOID VSTInterface_2X_WIN::dllCallback(LPVOID, int32 opcode, ...) {
	va_list arg;
	va_start(arg, opcode);
	LPCVOID res = nullptr;

	switch (opcode) {
	case 0: {
		int i1 = va_arg(arg, int);
		int32 i2 = va_arg(arg, int32);
		LPVOID i3 = va_arg(arg, LPVOID);
		float i4 = va_arg(arg, float);
		bool ggg=true; 
	}
		break;
	case 1:
		res = ConvertType<uint32>(2400).to<LPCVOID>();
		break;
	case 6:
		break;
	case 7:
		break;
	case 23:
		res = ConvertType<uint32>(2).to<LPCVOID>();
		break;
	case 41:
		res = _pluginFolder;
		break;
	default:
		warning("VSTInterface_2X_WIN::dllCallback(): Unsupported opcode '%d'", opcode);
		break;
	}

	va_end(arg);
	return res;
}

/*
class VSTInterface_30_WIN final : public VSTInterface {
public:
	VSTInterface_30_WIN(const Common::String &pluginPath);
	~VSTInterface_30_WIN() override;

private:
	Common::String _pluginPath;
	bool _ready;
};

VSTInterface_30_WIN::VSTInterface_30_WIN(const Common::String &pluginPath) : VSTInterface(), _pluginPath(pluginPath), _ready(false) {

}

VSTInterface_30_WIN::~VSTInterface_30_WIN() {

}

bool VSTInterface_30_WIN::loadPlugin() {

}

void VSTInterface_30_WIN::unloadPlugin() {

}

*/

VSTInterface *VSTInterface_WIN_create(const PluginInfo *target) {
	VSTInterface *res = nullptr;
	if (target->version == 3000)
		res = nullptr; /*new VSTInterface_30_WIN(target->path);*/
	else if (target->version >= 2000)
		res = new VSTInterface_2X_WIN(target->path);
	else
		error("VSTInterface_WIN_create(): Unsupported version %d for VST plugin '%s'", target->version, target->name.c_str());

	assert(res);

	return res;
}

#undef pluginCall
#undef pluginCallArgI32

} // end of namespace VST

#endif
