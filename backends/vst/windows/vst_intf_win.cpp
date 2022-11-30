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

#include "common/scummsys.h"
#include "common/system.h"

namespace VST {

#define pluginCall(a) if (_op_##a && _op_##a->isValid()) (*_op_##a)
#define pluginCallRes(a) !(_op_##a && _op_##a->isValid()) ? 0 : (*_op_##a)
#define pluginCallArgI32(a, val) pluginCall(a)(ConvertTypeLE<uint32>(val).to<LPVOID>())

class VSTInterface_2X_WIN final : public VSTInterface {
public:
	VSTInterface_2X_WIN(const Common::String &pluginPath, const Common::String &pluginName);
	~VSTInterface_2X_WIN() override;

	void setSampleRate(uint32 rate) override;
	void setBlockSize(uint32 bsize) override;
	void generateSamples(float **in, float **out, uint32 len, uint32 smpPos) override;
	void runEditor() override;

private:
	bool startPlugin() override;
	void terminatePlugin() override;
	void midiSendAll();

	void hold() const;
	void resume() const;

	const char *getSaveFileExt() const override;
	uint32 getActiveSettings(uint8 **dest) const override;
	void restoreSettings(const uint8 *data, uint32 dataSize) const override;
	uint32 getActiveParameters(uint32 **dest) const override;
	void restoreParameters(const uint32 *data, uint32 numPara) const override;

	HMODULE _plugin;
	LPVOID _sndWork;
	const uint8 *_msgBuffer;

	uint32 _rate;
	uint32 _bsize;

	HWND _hWnd;
	const Common::String _pluginPath;
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
	V2XFunctor3 *_op_readPara;
	V2XFunctor4 *_op_writePara;
	V2XFunctor1 *_op_readSettings;
	V2XFunctor1 *_op_writeSettings;
	V2XFunctor1 *_op_editStart;
	V2XFunctor1 *_op_editQuit;
	V2XFunctor1 *_op_editWinSize;
	V2XFunctor1 *_op_editTmrUpdt;

	Common::Array<V2XFunctor1*> _wndProcs;

	static LPCVOID dllCallback(LPVOID, int32 opcode, ...);
};

static const char *_pluginFolder = nullptr;
TCHAR winTitle[0x100];

VSTInterface_2X_WIN::VSTInterface_2X_WIN(const Common::String &pluginPath, const Common::String &pluginName) : VSTInterface(pluginName), _pluginPath(pluginPath), _plugin(nullptr), _sndWork(nullptr), _ready(false),
	_bsize(0), _op_close(nullptr), _op_stop(nullptr), _op_setRate(nullptr), _op_setBSize(nullptr), _op_sendMsg(nullptr), _op_setOnOff(nullptr), _op_genSamples(nullptr),
	/*_op_getParaDsc, */_op_readPara(nullptr), _op_writePara(nullptr), _op_readSettings(nullptr), _op_writeSettings(nullptr), _op_editStart(nullptr), _op_editQuit(nullptr),
	_op_editWinSize(nullptr), _op_editTmrUpdt(nullptr), _msgBuffer(nullptr), _hWnd(nullptr), _rate(0) {

	Common::String folder = _pluginPath.substr(0, _pluginPath.findLastOf('\\'));
	char *folderStr = new char[folder.size() + 1];
	assert(folderStr);
	Common::strlcpy(folderStr, folder.c_str(), folder.size() + 1);
	_pluginFolder = folderStr;

	TCHAR *title = Win32::stringToTchar(_pluginName + " Setup");
	_tcsncpy_s(winTitle, ARRAYSIZE(winTitle), title, _pluginName.size() + 6);
	free(title);
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

static uint32 _smpPos = 0;

void VSTInterface_2X_WIN::generateSamples(float **in, float **out, uint32 len, uint32 smpPos) {
	if (!_ready)
		return;
	_smpPos = smpPos;
	midiSendAll();
	pluginCall(genSamples)(in, out, len);
	clearChain();
}

LRESULT CALLBACK confWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	Common::Array<V2XFunctor1*> *plgFunc = reinterpret_cast<Common::Array<V2XFunctor1*>*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (uMsg) {
	case WM_CREATE:
		SetWindowLongPtr(hWnd, GWLP_USERDATA, *reinterpret_cast<LONG_PTR*>(lParam));
		SetTimer(hWnd, 1, 16, NULL);
		break;
	case WM_DESTROY:
		if (plgFunc && (*plgFunc)[0]->isValid()) {
			(*(*plgFunc)[0])();
			SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
			KillTimer(hWnd, 1);
		}
		break;
	case WM_TIMER:
		if (plgFunc && (*plgFunc)[1]->isValid())
			(*(*plgFunc)[1])();
		break;
	default: 
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}

	return 0;
}

void adjustWindow(HWND hwnd, int x1, int y1, int x2, int y2) {
#ifdef _DPI_AWARENESS_CONTEXTS_
	DPI_AWARENESS_CONTEXT oldDpiAC = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED);
#endif
	RECT wr = { x1, y1, x2, y2 };
	AdjustWindowRectEx(&wr, GetWindowLong(hwnd, GWL_STYLE), FALSE, GetWindowLong(hwnd, GWL_EXSTYLE));
	SetWindowPos(hwnd, HWND_TOP, 0, 0, wr.right - wr.left - 1, wr.bottom - wr.top - 1, SWP_NOMOVE);
	ShowWindow(hwnd, SW_SHOWNORMAL);
	UpdateWindow(hwnd);
#ifdef _DPI_AWARENESS_CONTEXTS_
	SetThreadDpiAwarenessContext(oldDpiAC);
#endif
}

void VSTInterface_2X_WIN::runEditor() {
	if (!_ready)
		return;

	TCHAR wname[] = _T("SndPluginConfClss");
	HMODULE mdl = GetModuleHandle(nullptr);
	HICON icn =  LoadIcon(mdl, MAKEINTRESOURCE(1001 /* IDI_ICON */));
	WNDCLASSEX wcl = { sizeof(WNDCLASSEX), 0,  &confWndProc, 0, 0, mdl, icn, LoadCursor(mdl, IDC_ARROW), (HBRUSH)(COLOR_WINDOW+1), nullptr, wname, icn };
	RegisterClassEx(&wcl);

#ifdef _DPI_AWARENESS_CONTEXTS_
	DPI_AWARENESS_CONTEXT oldDpiAC = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED);
#endif

	_hWnd =	CreateWindowEx(WS_EX_CLIENTEDGE, wname, winTitle, WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, mdl, &_wndProcs);
	
	pluginCall(editStart)(_hWnd);

	uint16 *rect = 0;
	pluginCall(editWinSize)(&rect);

	if (rect)
		adjustWindow(_hWnd, rect[1], rect[0], rect[3], rect[2]);

#ifdef _DPI_AWARENESS_CONTEXTS_
	SetThreadDpiAwarenessContext(oldDpiAC);
#endif
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
	_op_readPara = new V2XFunctor3(_sndWork, 4, 0);
	assert(_op_readPara);
	_op_writePara = new V2XFunctor4(_sndWork, 3, 0);
	assert(_op_writePara);
	_op_readSettings = new V2XFunctor1(_sndWork, 23, 4);
	assert(_op_readSettings);
	_op_writeSettings = new V2XFunctor1(_sndWork, 24, 6);
	assert(_op_writeSettings);
	_op_editStart = new V2XFunctor1(_sndWork, 14, 4);
	assert(_op_editStart);
	_op_editQuit = new V2XFunctor1(_sndWork, 15, 0);
	assert(_op_editQuit);
	_op_editWinSize = new V2XFunctor1(_sndWork, 13, 4);
	assert(_op_editWinSize);
	_op_editTmrUpdt = new V2XFunctor1(_sndWork, 19, 0);
	assert(_op_editTmrUpdt);

	_wndProcs.push_back(_op_editQuit);
	_wndProcs.push_back(_op_editTmrUpdt);

	_op_genSamples = new V2XFunctor2(_sndWork, 10, 10);
	assert(_op_genSamples);
	if (!_op_genSamples->isValid()) {
		delete _op_genSamples;
		// If the normal samples processing routine doesn't exist, we look for the older (deprecated) one.
		_op_genSamples = new V2XFunctor2(_sndWork, 2, 0);
		assert(_op_genSamples);
	}

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

	if (_hWnd && GetWindowLongPtr(_hWnd, GWLP_USERDATA))
		DestroyWindow(_hWnd);
	_hWnd = nullptr;

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
	delete _op_genSamples;
	_op_genSamples = nullptr;
	delete _op_editStart;
	_op_editStart = nullptr;
	delete _op_editQuit;
	_op_editQuit = nullptr;
	delete _op_editWinSize;
	_op_editWinSize = nullptr;
	delete _op_editTmrUpdt;
	_op_editTmrUpdt = nullptr;
}

static float _tempo = 120.0f;

void VSTInterface_2X_WIN::midiSendAll() {
	if (!_ready || !_eventsCount)
		return;

	delete[] _msgBuffer;
	uint32 buffSize = _eventsCount * (48 + sizeof(LPVOID)) + 3 * sizeof(LPVOID);
	uint8 *buff = new uint8[buffSize];
	memset(buff, 0, buffSize);
	uint8 *package = &buff[_eventsCount * 48];
	const uint8 **table = &(reinterpret_cast<const uint8**>(package))[_eventsCount + 1];
	uint8 *pos = buff;

	for (EvtNode *e = _eventsChain; e; e = e->_next) {
		if (e->_syx) {
			pos[0] = 6;
			pos[4] = (4 + sizeof(LPVOID)) << 2;
			*(reinterpret_cast<const uint8**>(&pos[(8 + sizeof(LPVOID)) << 1])) = e->_syx;
			WRITE_UINT32(&pos[16], e->_dat);
		} else if ((e->_dat & 0xFF) == 0xFF) {
			_tempo = 60000000.0f / (float)(e->_dat >> 8);
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

const char *VSTInterface_2X_WIN::getSaveFileExt() const {
	return "w2x";
}

void VSTInterface_2X_WIN::hold() const {
	pluginCallArgI32(setOnOff, 0);
}

void VSTInterface_2X_WIN::resume() const {
	pluginCallArgI32(setOnOff, 1);
}

uint32 VSTInterface_2X_WIN::getActiveSettings(uint8 **dest) const {
	const uint8 *plgSettings = nullptr;
	*dest = nullptr;
	if (!_ready)
		return 0;

	uint32 size = 0;
	if (plgProps(_sndWork, 4) & 0x20) {
		hold();
		size = pluginCallRes(readSettings)(&plgSettings);
		resume();
		uint8 *st = new uint8[size];
		memcpy(st, plgSettings, size);
		*dest = st;
	}

	return size;
}

void VSTInterface_2X_WIN::restoreSettings(const uint8 *data, uint32 dataSize) const {
	if (!_ready || !data || !dataSize)
		return;
	hold();
	pluginCall(writeSettings)(ConvertTypeLE<uint32>(dataSize).to<LPVOID>(), data);
	resume();
}

uint32 VSTInterface_2X_WIN::getActiveParameters(uint32 **dest) const {
	if (dest)
		*dest = nullptr;

	if (!_ready)
		return 0;

	uint32 num = plgProps(_sndWork, 1);
	if (!num)
		return 0;

	uint32 *res = new uint32[num]();

	hold();
	for (uint32 i = 0; i < num; ++i)
		res[i] = pluginCallRes(readPara)(i);
	resume();

	if (dest)
		*dest = res;	

	return num;
}

void VSTInterface_2X_WIN::restoreParameters(const uint32 *data, uint32 numPara) const {
	//if (!_ready || !data || !numPara)
		return;
	hold();
	for (uint32 i = 0; i < numPara; ++i)
		pluginCall(writePara)(i, data[i]);
	resume();
}

struct VstTimeInfo
{
	//-------------------------------------------------------------------------------------------------------
	double samplePos;				///< current Position in audio samples (always valid)
	double sampleRate;				///< current Sample Rate in Herz (always valid)
	double nanoSeconds;				///< System Time in nanoseconds (10^-9 second)
	double ppqPos;					///< Musical Position, in Quarter Note (1.0 equals 1 Quarter Note)
	double tempo;					///< current Tempo in BPM (Beats Per Minute)
	double barStartPos;				///< last Bar Start Position, in Quarter Note
	double cycleStartPos;			///< Cycle Start (left locator), in Quarter Note
	double cycleEndPos;				///< Cycle End (right locator), in Quarter Note
	uint32 timeSigNumerator;		///< Time Signature Numerator (e.g. 3 for 3/4)
	uint32 timeSigDenominator;	///< Time Signature Denominator (e.g. 4 for 3/4)
	uint32 smpteOffset;			///< SMPTE offset (in SMPTE subframes (bits; 1/80 of a frame)). The current SMPTE position can be calculated using #samplePos, #sampleRate, and #smpteFrameRate.
	uint32 smpteFrameRate;		///< @see VstSmpteFrameRate
	uint32 samplesToNextClock;	///< MIDI Clock Resolution (24 Per Quarter Note), can be negative (nearest clock)
	uint32 flags;					///< @see VstTimeInfoFlags
	//-------------------------------------------------------------------------------------------------------
};

static VstTimeInfo info = {};


LPCVOID VSTInterface_2X_WIN::dllCallback(LPVOID, int32 opcode, ...) {
	va_list arg;
	va_start(arg, opcode);
	LPCVOID res = nullptr;
	HWND hWnd = FindWindow(_T("SndPluginConfClss"), winTitle);

	struct VstTimeInfo
	{
		//-------------------------------------------------------------------------------------------------------
		double samplePos;				///< current Position in audio samples (always valid)
		double sampleRate;				///< current Sample Rate in Herz (always valid)
		double nanoSeconds;				///< System Time in nanoseconds (10^-9 second)
		double ppqPos;					///< Musical Position, in Quarter Note (1.0 equals 1 Quarter Note)
		double tempo;					///< current Tempo in BPM (Beats Per Minute)
		double barStartPos;				///< last Bar Start Position, in Quarter Note
		double cycleStartPos;			///< Cycle Start (left locator), in Quarter Note
		double cycleEndPos;				///< Cycle End (right locator), in Quarter Note
		uint32 timeSigNumerator;		///< Time Signature Numerator (e.g. 3 for 3/4)
		uint32 timeSigDenominator;	///< Time Signature Denominator (e.g. 4 for 3/4)
		uint32 smpteOffset;			///< SMPTE offset (in SMPTE subframes (bits; 1/80 of a frame)). The current SMPTE position can be calculated using #samplePos, #sampleRate, and #smpteFrameRate.
		uint32 smpteFrameRate;		///< @see VstSmpteFrameRate
		uint32 samplesToNextClock;	///< MIDI Clock Resolution (24 Per Quarter Note), can be negative (nearest clock)
		uint32 flags;					///< @see VstTimeInfoFlags
		//-------------------------------------------------------------------------------------------------------
	};

	uint32 rate = 48000;
	uint32 d = rate / 250;
	uint32 r = rate % 250;

	// This is equivalent to (getRate() << FIXP_SHIFT) / BASE_FREQ
	// but less prone to arithmetic overflow.

	uint32 samplesPerTick = (d << 16) + (r << 16) / 250;

	info = { 0.0, (double)rate, (double)g_system->getMillis() * 1000000.0, 0.0, (double)_tempo, 0.0, 0.0, 0.0, 1, 1, 0, 0, _smpPos, 0x8f02 };
	enum VstTimeInfoFlags
	{
		//-------------------------------------------------------------------------------------------------------
		kVstTransportChanged     = 1,		///< indicates that play, cycle or record state has changed
		kVstTransportPlaying     = 1 << 1,	///< set if Host sequencer is currently playing
		kVstTransportCycleActive = 1 << 2,	///< set if Host sequencer is in cycle mode
		kVstTransportRecording   = 1 << 3,	///< set if Host sequencer is in record mode
		kVstAutomationWriting    = 1 << 6,	///< set if automation write mode active (record parameter changes)
		kVstAutomationReading    = 1 << 7,	///< set if automation read mode active (play parameter changes)
		kVstNanosValid           = 1 << 8,	///< VstTimeInfo::nanoSeconds valid
		kVstPpqPosValid          = 1 << 9,	///< VstTimeInfo::ppqPos valid
		kVstTempoValid           = 1 << 10,	///< VstTimeInfo::tempo valid
		kVstBarsValid            = 1 << 11,	///< VstTimeInfo::barStartPos valid
		kVstCyclePosValid        = 1 << 12,	///< VstTimeInfo::cycleStartPos and VstTimeInfo::cycleEndPos valid
		kVstTimeSigValid         = 1 << 13,	///< VstTimeInfo::timeSigNumerator and VstTimeInfo::timeSigDenominator valid
		kVstSmpteValid           = 1 << 14,	///< VstTimeInfo::smpteOffset and VstTimeInfo::smpteFrameRate valid
		kVstClockValid           = 1 << 15	///< VstTimeInfo::samplesToNextClock valid
		//-------------------------------------------------------------------------------------------------------
	};

	

	switch (opcode) {
	case 0:
		break;
	case 1:
		res = ConvertTypeLE<uint32>(2400).to<LPCVOID>();
		break;
	case 6:
		break;
	case 7: {
		uint32 a = va_arg(arg, uint32);
		uint32 m = va_arg(arg, uint32);
		return &info;
	}
		break;
	case 15: {
		int w = va_arg(arg, int);
		int h = va_arg(arg, int);
		adjustWindow(hWnd, 0, 0, w, h);
		res = ConvertTypeLE<uint32>(1).to<LPCVOID>();
	}
		break;
	case 23:
		res = ConvertTypeLE<uint32>(2).to<LPCVOID>();
		break;
	case 41:
		res = _pluginFolder;
		break;
	case 42:
		UpdateWindow(hWnd);
		break;
	default:
		warning("VSTInterface_2X_WIN::dllCallback(): Unsupported opcode '%d'", opcode);
		break;
	}

	va_end(arg);
	return res;
}

class VSTInterface_3X_WIN final : public VSTInterface {
public:
	VSTInterface_3X_WIN(const Common::String &pluginPath, const Common::String &pluginName);
	~VSTInterface_3X_WIN() override;

	void setSampleRate(uint32 rate) override;
	void setBlockSize(uint32 bsize) override;
	void generateSamples(float **in, float **out, uint32 len, uint32 smpPos) override;
	void runEditor() override;

private:
	bool startPlugin() override;
	void terminatePlugin() override;

	const char *getSaveFileExt() const override;
	uint32 getActiveSettings(uint8 **dest) const override;
	void restoreSettings(const uint8 *data, uint32 dataSize) const override;
	uint32 getActiveParameters(uint32 **dest) const override;
	void restoreParameters(const uint32 *data, uint32 numPara) const override;

	Common::String _pluginPath;
	bool _ready;
};

void VSTInterface_3X_WIN::setSampleRate(uint32 rate) {

}

void VSTInterface_3X_WIN::setBlockSize(uint32 bsize) {

}

void VSTInterface_3X_WIN::generateSamples(float **in, float **out, uint32 len, uint32 smpPos) {

}

void VSTInterface_3X_WIN::runEditor() {

}

VSTInterface_3X_WIN::VSTInterface_3X_WIN(const Common::String &pluginPath, const Common::String &pluginName) : VSTInterface(pluginName), _pluginPath(pluginPath), _ready(false) {

}

VSTInterface_3X_WIN::~VSTInterface_3X_WIN() {

}

bool VSTInterface_3X_WIN::startPlugin() {

	return true;
}

void VSTInterface_3X_WIN::terminatePlugin() {

}

const char *VSTInterface_3X_WIN::getSaveFileExt() const {
	return "w3x";
}


uint32 VSTInterface_3X_WIN::getActiveSettings(uint8 **dest) const {
	const uint8 *plgSettings = nullptr;
	*dest = nullptr;
	if (!_ready)
		return 0;

	uint32 size = 0;

	return size;
}

void VSTInterface_3X_WIN::restoreSettings(const uint8 *data, uint32 dataSize) const {

}

uint32 VSTInterface_3X_WIN::getActiveParameters(uint32 **dest) const {
	if (dest)
		*dest = nullptr;

	if (!_ready)
		return 0;

	uint32 num = 0;
	if (!num)
		return 0;

	return num;
}

void VSTInterface_3X_WIN::restoreParameters(const uint32 *data, uint32 numPara) const {

}

VSTInterface *VSTInterface_WIN_create(const PluginInfo *target) {
	VSTInterface *res = nullptr;
	if (target->version >= 3000)
		res = new VSTInterface_3X_WIN(target->path, target->name);
	else if (target->version >= 2000)
		res = new VSTInterface_2X_WIN(target->path, target->name);
	else
		error("VSTInterface_WIN_create(): Unsupported version %d for VST plugin '%s'", target->version, target->name.c_str());

	assert(res);

	return res;
}

#undef pluginCall
#undef pluginCallArgI32

} // end of namespace VST

#endif
