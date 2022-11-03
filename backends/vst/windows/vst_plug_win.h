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

#ifndef BACKENDS_VSTPLUGWIN_H
#define BACKENDS_VSTPLUGWIN_H

#include <windows.h>
#include "common/scummsys.h"

namespace VST {

#define swrkPrc64Seek(a, x)		((FARPROC*)a)[x]
#define swrk32Seek(a, x, y)		&((uint32*)&swrkPrc64Seek(a, x))[y]
#define swrkPrc32Seek(a, x, y)	*(FARPROC*)swrk32Seek(a, x, y)
#define plg_prop(sndwork, a)	*(uint32*)swrk32Seek(sndwork, 5, a)

#define V2XFunctor1 V2XDLLFunctorEx<1, 0, int, int32, LPVOID, LPVOID, float>
#define V2XFunctor2 V2XDLLFunctor<void, float**, float**, uint32>

template <typename T_IN> struct ConvertType {
	ConvertType(T_IN arg) : _data(static_cast<LPCVOID>(&arg)) {}
	template <typename T_OUT> T_OUT to() const { return *(static_cast<const T_OUT*>(_data)); }
	LPCVOID _data;
};

template <typename RET, typename... TA> class V2XDLLFunctorBase {
public:
	V2XDLLFunctorBase(LPVOID sndWork, int offsPtr, int offs32) : _sndWork(sndWork), _proc(nullptr) {
		assert(_sndWork);
		_proc = (CBPROC)swrkPrc32Seek(_sndWork, offsPtr, offs32);
	}
	virtual ~V2XDLLFunctorBase() {}
	bool isValid() const { return (_proc && IS_ALIGNED(_proc, sizeof(FARPROC)) && !IsBadCodePtr((FARPROC)_proc)); }
protected:
	typedef RET (*CBPROC)(LPVOID, TA...);
	CBPROC _proc;
	const LPVOID _sndWork;
};

template <typename RET, typename... TA> class V2XDLLFunctor  : public V2XDLLFunctorBase<RET, TA...> {
public:
	V2XDLLFunctor(LPVOID sndWork, int offsPtr, int offs32) : V2XDLLFunctorBase(sndWork, offsPtr, offs32) {}
	~V2XDLLFunctor() override {}
	RET operator()(TA... args) { return _proc(_sndWork, args...); }
};

template <int OFFSPTR, int OFFS32, typename RET, typename TA1, typename TA2, typename TA3, typename TA4> class V2XDLLFunctorEx {
public:
	V2XDLLFunctorEx(LPVOID sndWork, int opcode, uint16 usageFlags) : _sndWork(sndWork), _opcode(opcode), _usageFlags(usageFlags), _proc(nullptr) {
		assert(_sndWork);
		_proc = (CBPROC)swrkPrc32Seek(_sndWork, OFFSPTR, OFFS32);
	}
	~V2XDLLFunctorEx() {}

	bool isValid() const { return (_proc && IS_ALIGNED(_proc, sizeof(FARPROC)) && !IsBadCodePtr((FARPROC)_proc)); }

	template <typename T> RET operator()(T arg, ...) {
		LPVOID pa = &arg;
		va_list arglst;
		va_start(arglst, arg);
/*
		TA ta;
		int ordr = 0;
		template<typename TAA>[&ordr] insert(TAA &taa) {
			taa = 0;
			if (_usageFlags & (1 << ordr)) {
				if (_usageFlags & ((1 << ordr) - 1)) {
					taa = va_arg(arglst, TAA);
				} else {
					assert(sizeof(T) == sizeof(TAA));
					b = *(static_cast<(TAA)*>(pa));
				}
			}
			++ordr;
		}

		int dummy[sizeof...(TA)] = {insert(ta)...};

		if (_usageFlags & (1 << ordr)) {
			if (_usageFlags & ((1 << ordr) - 1)) {
				b = va_arg(arglst, (TA...));
			} else {
				assert(sizeof(T) == sizeof(TA...));
				b = *(static_cast<(TA...)*>(pa));
			}
		}*/
		TA1 a = 0;
		TA2 b = 0;
		TA3 c = 0;
		TA4 d = 0;

		if (_usageFlags & 1) {
			assert(sizeof(T) == sizeof(TA1));
			a = *(static_cast<TA1*>(pa));
		}

		if (_usageFlags & 2) {
			if (_usageFlags & 1) {
				b = va_arg(arglst, TA2);
			} else {
				assert(sizeof(T) == sizeof(TA2));
				b = *(static_cast<TA2*>(pa));
			}
		}

		if (_usageFlags & 4) {
			if (_usageFlags & 3) {
				c = va_arg(arglst, TA3);
			} else {
				assert(sizeof(T) == sizeof(TA3));
				c = *(static_cast<TA3*>(pa));
			}
		}

		if (_usageFlags & 8) {
			if (_usageFlags & 7) {
				d = va_arg(arglst, TA4);
			} else {
				assert(sizeof(T) == sizeof(TA4));
				d = *(static_cast<TA4*>(pa));
			}
		}

		va_end(arglst);

		return ConvertType<LPCVOID>(_proc(_sndWork, _opcode, a, b, c, d)).to<RET>();
	}

	RET operator()() {
		assert(_usageFlags == 0);
		return ConvertType<LPCVOID>(_proc(_sndWork, _opcode, 0, 0, 0, 0)).to<RET>();
	}

private:
	typedef LPCVOID (*CBPROC)(LPVOID, int32, TA1, TA2, TA3, TA4);
	const LPVOID _sndWork;
	const int32 _opcode;
	const uint16 _usageFlags;
	CBPROC _proc;
};

typedef LPCVOID (*hostCBProc)(LPVOID, int32, ...);
bool loadV2xPlugin(const TCHAR *dllPath, hostCBProc proc, HMODULE &destModule, LPVOID &destSndWork);
void unloadV2xPlugin(HMODULE module);

struct plgProps {
	plgProps(LPVOID sndWork, int a) : _prop(sndWork ? plg_prop(sndWork, a) : 0) {}
	operator int() const { return _prop; }
	uint32 _prop;
};

#undef swrkPrc64Seek
#undef swrk32Seek
#undef swrkPrc32Seek
#undef plg_flags

} // end of namespace VST

#endif

#endif
