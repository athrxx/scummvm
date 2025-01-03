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


#include "common/system.h"
#include "graphics/cursorman.h"
#include "sci/graphics/drivers/gfxdriver_intern.h"

namespace Sci {

class SCI0_MacGfxDriver final : public GfxDefaultDriver {
public:
	SCI0_MacGfxDriver(uint16 screenWidth, uint16 screenHeight, bool rgbRendering);
	~SCI0_MacGfxDriver() override;
	void replaceMacCursor(const Graphics::Cursor *cursor) override;
private:
};

SCI0_MacGfxDriver::SCI0_MacGfxDriver(uint16 screenWidth, uint16 screenHeight, bool rgbRendering) : GfxDefaultDriver(screenWidth, screenHeight, true, rgbRendering) {
	if (!_compositeBuffer)
		_compositeBuffer = new byte[64 * 64 * _pixelSize]();
	_cursorUsesScreenPalette = false;
}

SCI0_MacGfxDriver::~SCI0_MacGfxDriver() {
}

void SCI0_MacGfxDriver::replaceMacCursor(const Graphics::Cursor*) {
	// This is not needed for SCI0 (and not for any PC version of games at all)
	error("%s(Graphics::Cursor*): Not implemented", __FUNCTION__);
}

GfxDriver *SCI0_MacGfxDriver_create(int rgbRendering, ...) {
	va_list args;
	va_start(args, rgbRendering);
	va_arg(args, int);
	int width = va_arg(args, int);
	int height = va_arg(args, int);
	va_end(args);

	return new SCI0_MacGfxDriver(width, height, rgbRendering != 0);
};

class SCI1_MacGfxDriver final : public GfxDefaultDriver {
public:
	SCI1_MacGfxDriver(uint16 screenWidth, uint16 screenHeight, bool rgbRendering);
	~SCI1_MacGfxDriver() override;
	void initScreen(const Graphics::PixelFormat *format) override;
	void replaceCursor(const void*, uint, uint, int, int, uint32) override;
	void replaceMacCursor(const Graphics::Cursor *cursor) override;
private:
};

SCI1_MacGfxDriver::SCI1_MacGfxDriver(uint16 screenWidth, uint16 screenHeight, bool rgbRendering) : GfxDefaultDriver(screenWidth, screenHeight, false, rgbRendering) {
	_cursorUsesScreenPalette = false;
}

SCI1_MacGfxDriver::~SCI1_MacGfxDriver() {
}

void SCI1_MacGfxDriver::initScreen(const Graphics::PixelFormat *format) {
	GfxDefaultDriver::initScreen(format);
	if (!_compositeBuffer)
		_compositeBuffer = new byte[_screenW * _screenH * _pixelSize]();
}

void SCI1_MacGfxDriver::replaceCursor(const void*, uint, uint, int, int, uint32) {
	// This is not needed for SCI1 Mac versions of games.
	error("SCI1_MacUpscaledGfxDriver::replaceCursor(const void*, uint, uint, int, int, uint32): Not implemented");
}

void SCI1_MacGfxDriver::replaceMacCursor(const Graphics::Cursor *c) {
	GFXDRV_ASSERT_READY;

	if (!c || !c->getSurface())
		error("SCI1_MacUpscaledGfxDriver::replaceMacCursor(): Invalid cursor");

	uint16 w = c->getWidth();
	uint16 h = c->getHeight();
	uint16 hotX = c->getHotspotX();
	uint16 hotY = c->getHotspotY();

	SciGfxDrvInternal::scale2x<byte>(_compositeBuffer, c->getSurface(), w, w, h);
	w <<= 1;
	h <<= 1;
	hotX <<= 1;
	hotY <<= 1;

	CursorMan.replaceCursor(_compositeBuffer, w, h, hotX, hotY, c->getKeyColor(), false, nullptr, c->getMask());
	if (c->getPalette())
		CursorMan.replaceCursorPalette(c->getPalette(), c->getPaletteStartIndex(), c->getPaletteCount());
}

GfxDriver *SCI1_MacGfxDriver_create(int rgbRendering, ...) {
	va_list args;
	va_start(args, rgbRendering);
	va_arg(args, int);
	int width = va_arg(args, int);
	int height = va_arg(args, int);
	va_end(args);

	return new SCI1_MacGfxDriver(width, height, rgbRendering != 0);
}

class SCI_KOR_GfxDriver final : public UpscaledGfxDriver {
public:
	SCI_KOR_GfxDriver(bool rgbRendering);
	~SCI_KOR_GfxDriver() override {}

	void drawTextFontGlyph(const byte *src, int pitch, int hiresDestX, int hiresDestY, int hiresW, int hiresH, int transpColor, const PaletteMod *palMods, const byte *palModMapping) override;
	void setFlags(uint32 flags) override;
	void clearFlags(uint32 flags) override;

private:
	uint32 _flags;
};

SCI_KOR_GfxDriver::SCI_KOR_GfxDriver(bool rgbRendering) : UpscaledGfxDriver(640, 400, 1, true, rgbRendering), _flags(0) {

}

void SCI_KOR_GfxDriver::drawTextFontGlyph(const byte *src, int pitch, int hiresDestX, int hiresDestY, int hiresW, int hiresH, int transpColor, const PaletteMod *palMods, const byte *palModMapping) {

}

void SCI_KOR_GfxDriver::setFlags(uint32 flags) {
	flags ^= (_flags & flags);
	if (!flags)
		return;

//	if (flags & kMovieMode)
//		_renderLine2 = _smallWindow ? &smallWindowRenderLineMovie : &largeWindowRenderLineMovie;

	_flags |= flags;
}

void SCI_KOR_GfxDriver::clearFlags(uint32 flags) {
	flags &= _flags;
	if (!flags)
		return;

//	if (flags & kMovieMode)
//		_renderLine2 = _smallWindow ? &renderLineDummy : &hiresRenderLine;

	_flags &= ~flags;
}

GfxDriver *SCI_KOR_Gfx_create(int rgbRendering, ...) {
	return new SCI_KOR_GfxDriver(rgbRendering != 0);
}

} // End of namespace Sci
