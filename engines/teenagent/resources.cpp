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

#include "engines/advancedDetector.h"

#include "teenagent/resources.h"
#include "teenagent/teenagent.h"
#include "common/debug.h"
#include "common/textconsole.h"
#include "common/translation.h"
#include "common/compression/deflate.h"

namespace TeenAgent {

Resources::Resources() {
}

Resources::~Resources() {
	off.close();
	on.close();
	ons.close();
	lan000.close();
	lan500.close();
	mmm.close();
	sam_mmm.close();
	sam_sam.close();
	voices.close();
}

/*
quick note on varia resources:
1: Mark's animations (with head)
2: Mark's idle animation
3: Inventory background
4: Inventory items
5: Metropolis palette
6: TEENAGENT logo (flames)
7: Small font
8: Bigger font
9: Metropolis software house
10: quit registered
11: quit shareware
*/

#define CSEG_SIZE 46000 // 0xb3b0
#define DSEG_SIZE 59280 // 0xe790
#define ESEG_SIZE 35810 // 0x8be2

void Resources::precomputeResourceOffsets(Segment &seg, Common::Array<uint16> &offsets, uint numTerminators) {
	offsets.push_back(0);
	uint n = 0;
	uint8 current, last = 0xff;
	for (uint i = 0; i < seg.size(); i++) {
		current = seg.get_byte(i);

		if (n == numTerminators) {
			offsets.push_back(i);
			n = 0;
		}

		if (current != 0x00 && last == 0x00)
			n = 0;

		if (current == 0x00)
			n++;

		last = current;
	}
}

void Resources::precomputeDialogOffsets() {
	precomputeResourceOffsets(eseg, dialogOffsets, 4);

	debug(1, "Resources::precomputeDialogOffsets() - Found %d dialogs", dialogOffsets.size());
	for (uint i = 0; i < dialogOffsets.size(); i++)
		debug(1, "\tDialog #%d: Offset 0x%04x", i, dialogOffsets[i]);
}

void Resources::precomputeCreditsOffsets() {
	precomputeResourceOffsets(creditsSeg, creditsOffsets);

	debug(1, "Resources::precomputeCreditsOffsets() - Found %d credits", creditsOffsets.size());
	for (uint i = 0; i < creditsOffsets.size(); i++)
		debug(1, "\tCredit #%d: Offset 0x%04x", i, creditsOffsets[i]);
}

void Resources::precomputeItemOffsets() {
	precomputeResourceOffsets(itemsSeg, itemOffsets);

	debug(1, "Resources::precomputeItemOffsets() - Found %d items", itemOffsets.size());
	for (uint i = 0; i < itemOffsets.size(); i++)
		debug(1, "\tItem #%d: Offset 0x%04x", i, itemOffsets[i]);
}

void Resources::precomputeMessageOffsets() {
	precomputeResourceOffsets(messagesSeg, messageOffsets);
}

void Resources::precomputeCombinationOffsets() {
	precomputeResourceOffsets(combinationsSeg, combinationOffsets);

	debug(1, "Resources::precomputeCombinationOffsets() - Found %d combination items", combinationOffsets.size());
	for (uint i = 0; i < combinationOffsets.size(); i++)
		debug(1, "\tCombination #%d: Offset 0x%04x", i, combinationOffsets[i]);
}

void Resources::readDialogStacks(byte *src) {
	uint16 base = dsAddr_dialogStackPleadingToMansionGuard;

	byte dialogStackWritten = 0;
	uint i = 0;

	while (dialogStackWritten < kNumDialogStacks) {
		uint16 word = READ_LE_UINT16(src + i * 2);
		dseg.set_word(base + i * 2, word);
		if (word == 0xFFFF)
			dialogStackWritten++;
		i++;
	}
}

bool Resources::loadArchives(const ADGameDescription *gd) {
	Common::File *dat_file = new Common::File();
	Common::String filename = "teenagent.dat";
	if (!dat_file->open(filename.c_str())) {
		delete dat_file;

		const char *msg = _s("Unable to locate the '%s' engine data file.");
		Common::U32String errorMessage = Common::U32String::format(_(msg), filename.c_str());
		warning(msg, filename.c_str());
		GUIErrorMessage(errorMessage);
		return false;
	}

	// teenagent.dat used to be compressed with zlib compression. The usage of
	// zlib here is no longer needed, and it's maintained only for backwards
	// compatibility.
	Common::SeekableReadStream *dat = Common::wrapCompressedReadStream(dat_file);

	byte tempBuffer[256];
	dat->read(tempBuffer, 9);
	tempBuffer[9] = '\0';

	if (strcmp((char *)tempBuffer, "TEENAGENT") != 0) {
		const char *msg = _s("The '%s' engine data file is corrupt.");
		Common::U32String errorMessage = Common::U32String::format(_(msg), filename.c_str());
		GUIErrorMessage(errorMessage);
		warning(msg, filename.c_str());
		return false;
	}

	byte version = dat->readByte();
	if (version != TEENAGENT_DAT_VERSION) {
		const char *msg = _s("Incorrect version of the '%s' engine data file found. Expected %d but got %d.");
		Common::U32String errorMessage = Common::U32String::format(_(msg), filename.c_str(), TEENAGENT_DAT_VERSION, version);
		GUIErrorMessage(errorMessage);
		warning(msg, filename.c_str());
		return false;
	}

	dat->skip(CSEG_SIZE);
	dseg.read(dat, DSEG_SIZE);

	// Locate the correct language block
	bool found = false;

	while (!found) {
		dat->read(tempBuffer, 5);
		if (tempBuffer[0] == 0xff) {
			error("Could not locate correct language block");
		}

		if (gd->language == tempBuffer[0]) {
			found = true;
			uint32 dataOffset = READ_LE_UINT32(&tempBuffer[1]);
			dat->seek(dataOffset);
		}
	}

	uint resourceSize = dat->readUint32LE();
	eseg.read(dat, resourceSize);

	// Dialog stack data
	resourceSize = dat->readUint32LE();
	dat->read(tempBuffer, resourceSize);
	readDialogStacks((byte *)tempBuffer);

	resourceSize = dat->readUint32LE();
	itemsSeg.read(dat, resourceSize);

	resourceSize = dat->readUint32LE();
	creditsSeg.read(dat, resourceSize);

	resourceSize = dat->readUint32LE();
	sceneObjectsSeg.read(dat, resourceSize);

	resourceSize = dat->readUint32LE();
	messagesSeg.read(dat, resourceSize);

	resourceSize = dat->readUint32LE();
	combinationsSeg.read(dat, resourceSize);

	delete dat;

	precomputeDialogOffsets();
	precomputeItemOffsets();
	precomputeCreditsOffsets();
	precomputeMessageOffsets();
	precomputeCombinationOffsets();

	FilePack varia;
	varia.open("varia.res");
	font7.load(varia, 7, 11, 1);
	font8.load(varia, 8, 31, 0);
	varia.close();

	off.open("off.res");
	on.open("on.res");
	ons.open("ons.res");
	lan000.open("lan_000.res");
	lan500.open("lan_500.res");
	mmm.open("mmm.res");
	sam_mmm.open("sam_mmm.res");
	sam_sam.open("sam_sam.res");
	voices.open("voices.res");

	return true;
}

void Resources::loadOff(Graphics::Surface &surface, byte *palette, int id) {
	uint32 size = off.getSize(id);
	if (size == 0) {
		error("invalid background %d", id);
		return;
	}

	const uint bufferSize = 64768;
	byte *buf = (byte *)malloc(bufferSize);
	if (!buf)
		error("[Resources::loadOff] Cannot allocate buffer");

	off.read(id, buf, bufferSize);

	byte *src = buf;
	byte *dst = (byte *)surface.getPixels();
	memcpy(dst, src, 64000);
	memcpy(palette, buf + 64000, 768);

	free(buf);
}

Common::SeekableReadStream *Resources::loadLan(uint32 id) const {
	return id <= 500 ? loadLan000(id) : lan500.getStream(id - 500);
}

Common::SeekableReadStream *Resources::loadLan000(uint32 id) const {
	switch (id) {
	case 81:
		if (dseg.get_byte(dsAddr_dogHasBoneFlag))
			return lan500.getStream(160);
		break;

	case 137:
		if (dseg.get_byte(dsAddr_mansionTVOnFlag) == 1) {
			if (dseg.get_byte(dsAddr_mansionVCRPlayingTapeFlag) == 1)
				return lan500.getStream(203);
			else
				return lan500.getStream(202);
		}
		break;

	case 25:
		if (dseg.get_byte(dsAddr_FirstActTrialState) == 2) {
			return lan500.getStream(332);
		}
		break;

	case 37:
		if (dseg.get_byte(dsAddr_act1GuardState) == 1) {
			return lan500.getStream(351);
		} else if (dseg.get_byte(dsAddr_act1GuardState) == 2) {
			return lan500.getStream(364);
		}
		break;

	case 29:
		if (dseg.get_byte(dsAddr_birdOnBarRadioAntennaFlag) == 1) {
			return lan500.getStream(380);
		}
		break;

	case 30:
		if (dseg.get_byte(dsAddr_birdOnBarRadioAntennaFlag) == 1) {
			return lan500.getStream(381);
		}
		break;

	case 42:
		if (dseg.get_byte(dsAddr_johnNotyOutsideMansionDoorFlag) == 1) {
			return lan500.getStream(400);
		}
		break;

	default:
		break;
	}
	return lan000.getStream(id);
}

} // End of namespace TeenAgent
