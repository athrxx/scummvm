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

#include "backends/vst/vst_detect.h"
#include "backends/vst/vst_intf.h"
#include "common/scummsys.h"

namespace VST {

class VSTInterface_24_WIN final : public VSTInterface {
public:
	VSTInterface_24_WIN();
	~VSTInterface_24_WIN() override;

private:
	bool _ready;
};

VSTInterface_24_WIN::VSTInterface_24_WIN() : VSTInterface() {

}

VSTInterface_24_WIN::~VSTInterface_24_WIN() {

}

class VSTInterface_30_WIN final : public VSTInterface {
public:
	VSTInterface_30_WIN();
	~VSTInterface_30_WIN() override;

private:
	bool _ready;
};

VSTInterface_30_WIN::VSTInterface_30_WIN() : VSTInterface() {

}

VSTInterface_30_WIN::~VSTInterface_30_WIN() {

}

VSTInterface *VSTInterface_WIN_create(const PluginInfo *target) {
	VSTInterface *res = nullptr;
	if (target->version == 24)
		res = new VSTInterface_24_WIN();
	//else if (target->version == 30)
	//	res = new VSTInterface_30_WIN();
	else 
		error("VSTInterface_WIN_create(): Unsupported version %d for VST plugin '%s'", target->version, target->name);

	assert(res);

	return res;
}

} // end of namespace VST

#endif
