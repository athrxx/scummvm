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

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "graphics/font.h"

namespace Graphics {

class MultiFont : public Font {
public:
	MultiFont();
	~MultiFont() override;

	typedef Font*(*FontLoaderProc)();

	enum LoadFlags : uint32 {
		kPrimary = 1,		// Primary font that applies if there is no match for any defined subset or if a font method is called that does not handle a specific codepoint.
		kOnDemand = 2,		// Load the font only when it is needed.
		kKeepResident = 4,	// Keep the font in memory after it has been loaded.
		kImmediate = 4		// Load the font immediately and keep it in memory.
	};

	enum RangeLimits : Common::U32String::unsigned_type {
		kMin = 0,
		kMax = 0xFFFFFFFF,
		kInvalid = 0xFFFFFFFF
	};

	struct SubsetRange {
		SubsetRange(Common::U32String::unsigned_type first = kMin, Common::U32String::unsigned_type last = kMax) : rfirst(first), rlast(last) {}
		Common::U32String::unsigned_type rfirst;
		Common::U32String::unsigned_type rlast;
	};

	typedef Common::Array<SubsetRange> SubsetRanges;

	void defineSubset(SubsetRanges &ranges, FontLoaderProc *loader, LoadFlags lflags);

	int getFontHeight() const override;
	Common::String getFontName() const override;
	int getFontAscent() const override;
	int getMaxCharWidth() const override;
	int getCharWidth(uint32 chr) const override;
	int getKerningOffset(uint32 left, uint32 right) const override;
	Common::Rect getBoundingBox(uint32 chr) const override;
	void drawChar(Surface *dst, uint32 chr, int x, int y, uint32 color) const override;
	void drawChar(ManagedSurface *dst, uint32 chr, int x, int y, uint32 color) const override;

private:
	struct Subset {
		Subset(SubsetRanges rngs, FontLoaderProc &ldr, uint32 flg) : ranges(rngs), loader(ldr), flags(flg), font(nullptr) {}
		~Subset() { delete font; }
		bool contains(uint32 chr) const {
			for (SubsetRanges::const_iterator i = ranges.begin(); i != ranges.end(); ++i) {
				if (chr >= i->rfirst && chr <= i->rlast)
					return true;
			}
			return false;
		}
		SubsetRanges ranges;
		FontLoaderProc loader;
		uint32 flags;
		mutable Font *font;

		struct Fnt {
			Fnt(const Subset *s) : sub(s) {}
			Fnt() : sub(nullptr) {}
			Font *operator()() const { return sub->font; }
			~Fnt() {
				if (!(sub->flags & kKeepResident)) {
					delete sub->font;
					sub->font = nullptr;
				}
			}
			const Subset *sub;
		};
	};

	Subset::Fnt getSubsetFont(Common::U32String::unsigned_type chr = kMax) const;

	Common::Array<Subset> _subsets;
};

MultiFont::MultiFont() {

}

MultiFont::~MultiFont() {
}

void MultiFont::defineSubset(SubsetRanges &ranges, FontLoaderProc *loader, LoadFlags lflags) {
	for (SubsetRanges::const_iterator i = ranges.begin(); i != ranges.end(); ++i) {
		if (i->rfirst > i->rlast)
			error("%s(): Invalid subset range: 0x%08x - 0x%08x", __FUNCTION__, i->rfirst, i->rlast);
		for (Common::Array<Subset>::const_iterator ii = _subsets.begin(); ii != _subsets.end(); ++ii) {
			if ((ii->contains(i->rfirst) || ii->contains(i->rlast)) && !(ii->flags & kPrimary) && !(lflags & kPrimary))
				error("%s(): Overlapping subset ranges are not allowed unless one of them is flagged as 'kPrimary'", __FUNCTION__);
			if ((lflags & kPrimary) && (ii->flags & kPrimary))
				error("%s(): Only one primary subset is allowed", __FUNCTION__);
		}
	}

	if (!loader)
		error("%s(): Invalid font loader function passed", __FUNCTION__);
	Subset s(ranges, *loader, lflags & (kPrimary | kKeepResident));

	if (!(lflags & kOnDemand))
		s.font = s.loader();

	_subsets.push_back(s);
}

int MultiFont::getFontHeight() const {
	Subset::Fnt fnt = getSubsetFont(kInvalid);
	return fnt() ? fnt()->getFontHeight() : 0;
}

Common::String MultiFont::getFontName() const {
	Subset::Fnt fnt = getSubsetFont(kInvalid);
	return fnt() ? fnt()->getFontName() : Common::String();
}

int MultiFont::getFontAscent() const {
	Subset::Fnt fnt = getSubsetFont(kInvalid);
	return fnt() ? fnt()->getFontAscent() : 0;
}

int MultiFont::getMaxCharWidth() const {
	Subset::Fnt fnt = getSubsetFont(kInvalid);
	return fnt() ? fnt()->getMaxCharWidth() : 0;
}

int MultiFont::getCharWidth(uint32 chr) const {
	Subset::Fnt fnt = getSubsetFont(chr);
	return fnt() ? fnt()->getCharWidth(chr) : 0;
}

int MultiFont::getKerningOffset(uint32 left, uint32 right) const {
	Subset::Fnt fnt = getSubsetFont(kInvalid);
	return fnt() ? fnt()->getKerningOffset(left, right) : 0;
}

Common::Rect MultiFont::getBoundingBox(uint32 chr) const {
	Subset::Fnt fnt = getSubsetFont(chr);
	return fnt() ? fnt()->getBoundingBox(chr) : Common::Rect();
}

void MultiFont::drawChar(Surface *dst, uint32 chr, int x, int y, uint32 color) const {
	Subset::Fnt fnt = getSubsetFont(chr);
	if (fnt())
		fnt()->drawChar(dst, chr, x, y, color);
}

void MultiFont::drawChar(ManagedSurface *dst, uint32 chr, int x, int y, uint32 color) const {
	Subset::Fnt fnt = getSubsetFont(chr);
	if (fnt())
		fnt()->drawChar(dst, chr, x, y, color);
}

MultiFont::Subset::Fnt MultiFont::getSubsetFont(uint32 chr) const {
	Common::Array<Subset>::const_iterator res = _subsets.end();
	Common::Array<Subset>::const_iterator resAlt = _subsets.end();
	for (Common::Array<Subset>::const_iterator i = _subsets.begin(); res == _subsets.end() && i != _subsets.end(); ++i) {
		if (i->contains(chr))
			res = i;
		else if (res == _subsets.end() && (i->flags & kPrimary))
			resAlt = i;
	}

	if (res == _subsets.end())
		res = resAlt;

	if (res != _subsets.end()) {
		if (!res->font)
			res->font = res->loader();
		return Subset::Fnt(res);
	}

	return Subset::Fnt();
}

	
} // End of namespace Graphics

