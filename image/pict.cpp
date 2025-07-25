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

#include "image/pict.h"
#include "image/codecs/codec.h"

#include "common/bitstream.h"
#include "common/debug.h"
#include "common/endian.h"
#include "common/memstream.h"
#include "common/stream.h"
#include "common/substream.h"
#include "common/textconsole.h"
#include "graphics/surface.h"

namespace Image {

// The PICT code is based off of the QuickDraw specs:
// https://developer.apple.com/library/archive/documentation/mac/QuickDraw/QuickDraw-461.html
// https://developer.apple.com/library/archive/documentation/mac/QuickDraw/QuickDraw-269.html

PICTDecoder::PICTDecoder() : _outputSurface(nullptr), _palette(0), _version(2) {
}

PICTDecoder::~PICTDecoder() {
	destroy();
}

void PICTDecoder::destroy() {
	if (_outputSurface) {
		_outputSurface->free();
		delete _outputSurface;
		_outputSurface = nullptr;
	}

	_palette.clear();
}

#define OPCODE(a, b, c) _opcodes.push_back(PICTOpcode(a, &PICTDecoder::b, c))

void PICTDecoder::setupOpcodesCommon() {
	OPCODE(0x0000, o_nop, "NOP");
	OPCODE(0x0001, o_clip, "Clip");
	OPCODE(0x0003, o_txFont, "TxFont");
	OPCODE(0x0004, o_txFace, "TxFace");
	OPCODE(0x0007, o_pnSize, "PnSize");
	OPCODE(0x0009, o_pnPat, "PnPat");
	OPCODE(0x000D, o_txSize, "TxSize");
	OPCODE(0x0010, o_txRatio, "TxRatio");
	OPCODE(0x0011, o_versionOp, "VersionOp");
	OPCODE(0x001E, o_nop, "DefHilite");
	OPCODE(0x0022, o_shortLine, "ShortLine");
	OPCODE(0x0023, o_shortLineFrom, "ShortLineFrom");
	OPCODE(0x0028, o_longText, "LongText");
	OPCODE(0x0091, o_bitsRgn, "BitsRgn");
	OPCODE(0x0099, o_packBitsRgn, "PackBitsRgn");
	OPCODE(0x00A0, o_shortComment, "ShortComment");
	OPCODE(0x00A1, o_longComment, "LongComment");
	OPCODE(0x00FF, o_opEndPic, "OpEndPic");
	OPCODE(0x0C00, o_headerOp, "HeaderOp");
	OPCODE(0x1101, o_versionOp1, "VersionOp1");
}

void PICTDecoder::setupOpcodesNormal() {
	setupOpcodesCommon();
	OPCODE(0x0090, on_bitsRect, "BitsRect");
	OPCODE(0x0098, on_packBitsRect, "PackBitsRect");
	OPCODE(0x009A, on_directBitsRect, "DirectBitsRect");
	OPCODE(0x8200, on_compressedQuickTime, "CompressedQuickTime");
}

void PICTDecoder::setupOpcodesQuickTime() {
	setupOpcodesCommon();
	OPCODE(0x0098, oq_packBitsRect, "PackBitsRect");
	OPCODE(0x009A, oq_directBitsRect, "DirectBitsRect");
	OPCODE(0x8200, oq_compressedQuickTime, "CompressedQuickTime");
}

#undef OPCODE

void PICTDecoder::o_nop(Common::SeekableReadStream &) {
	// Nothing to do
}

void PICTDecoder::o_clip(Common::SeekableReadStream &stream) {
	// Ignore
	int size = stream.readUint16BE();
	debugC(3, kDebugLevelGGraphics, "CLIP: size is %d", size);
	if (size >= 10) {
		int x1 = stream.readSint16BE();
		int y1 = stream.readSint16BE();
		int x2 = stream.readSint16BE();
		int y2 = stream.readSint16BE();
		debugC(3, kDebugLevelGGraphics, "CLIP: RECT encountered: %d %d %d %d", x1, y1, x2, y2);
		stream.skip(size - 10);
		debugC(3, kDebugLevelGGraphics, "CLIP: skipped %d bytes", size - 10);
	} else {
		stream.skip(size - 2);
		debugC(3, kDebugLevelGGraphics, "CLIP: skipped %d bytes", size - 2);
	}
}

void PICTDecoder::o_txFont(Common::SeekableReadStream &stream) {
	// Ignore
	stream.readUint16BE();
}

void PICTDecoder::o_txFace(Common::SeekableReadStream &stream) {
	// Ignore
	stream.readByte();
}

void PICTDecoder::o_pnSize(Common::SeekableReadStream &stream) {
	// Ignore
	stream.readUint16BE();
	stream.readUint16BE();
}

void PICTDecoder::o_pnPat(Common::SeekableReadStream &stream) {
	for (int i = 0; i < 8; i++) {
		_penPattern[i] = stream.readByte();
	}
}

void PICTDecoder::o_txSize(Common::SeekableReadStream &stream) {
	// Ignore
	stream.readUint16BE();
}

void PICTDecoder::o_txRatio(Common::SeekableReadStream &stream) {
	// Ignore
	stream.readUint16BE();
	stream.readUint16BE();
	stream.readUint16BE();
	stream.readUint16BE();
}

void PICTDecoder::o_versionOp(Common::SeekableReadStream &stream) {
	// We only support v2 extended
	if (stream.readUint16BE() != 0x02FF)
		error("Unknown PICT version");
	else
		_version = 2;
}

void PICTDecoder::o_versionOp1(Common::SeekableReadStream& stream) {
	_version = 1;
}

void PICTDecoder::o_shortLine(Common::SeekableReadStream &stream) {
	// Read the pen location (pnLoc)
	int16 pnLocX = stream.readSint16BE();
	int16 pnLocY = stream.readSint16BE();

	// Update the current pen position
	_currentPenPosition.x = pnLocX;
	_currentPenPosition.y = pnLocY;

	// Read the relative coordinates for the end of the line (dh, dv)
	int8 dh = stream.readByte(); // Delta horizontal
	int8 dv = stream.readByte(); // Delta vertical

	// Calculate the end position of the line
	int16 endX = pnLocX + dh;
	int16 endY = pnLocY + dv;

	// Draw the line from the current pen location to the new position
	//drawLine(pnLocX, pnLocY, endX, endY);

	// Update the pen position
	_currentPenPosition.x = endX;
	_currentPenPosition.y = endY;
}

void PICTDecoder::o_shortLineFrom(Common::SeekableReadStream &stream) {
	// Read the relative coordinates (dh, dv)
	int8 dh = stream.readByte(); // Delta horizontal
	int8 dv = stream.readByte(); // Delta vertical

	// Calculate the new pen position
	int16 newX = _currentPenPosition.x + dh;
	int16 newY = _currentPenPosition.y + dv;

	// Draw the line from the current pen position to the new position
	//drawLine(_currentPenPosition.x, _currentPenPosition.y, newX, newY);

	// Update the pen position
	_currentPenPosition.x = newX;
	_currentPenPosition.y = newY;
}

void PICTDecoder::o_longText(Common::SeekableReadStream &stream) {
	// Ignore
	stream.readUint16BE();
	stream.readUint16BE();
	stream.skip(stream.readByte());
}

void PICTDecoder::o_bitsRgn(Common::SeekableReadStream &stream) {
	// Copy unpacked data with clipped region (8bpp or lower)
	unpackBitsRectOrRgn(stream, false, true);
}

void PICTDecoder::o_packBitsRgn(Common::SeekableReadStream &stream) {
	unpackBitsRectOrRgn(stream, true, true);
}

void PICTDecoder::o_shortComment(Common::SeekableReadStream &stream) {
	// Ignore
	stream.readUint16BE();
}

void PICTDecoder::o_longComment(Common::SeekableReadStream &stream) {
	// Ignore
	stream.readUint16BE();
	stream.skip(stream.readUint16BE());
}

void PICTDecoder::o_opEndPic(Common::SeekableReadStream &stream) {
	// We've reached the end of the picture
	_continueParsing = false;
}

void PICTDecoder::o_headerOp(Common::SeekableReadStream &stream) {
	// Read the basic header, but we don't really have to do anything with it
	/* uint16 version = */ stream.readUint16BE();
	stream.readUint16BE(); // Reserved
	/* uint32 hRes = */ stream.readUint32BE();
	/* uint32 vRes = */ stream.readUint32BE();
	Common::Rect origResRect;
	origResRect.top = stream.readUint16BE();
	origResRect.left = stream.readUint16BE();
	origResRect.bottom = stream.readUint16BE();
	origResRect.right = stream.readUint16BE();
	stream.readUint32BE(); // Reserved
}

void PICTDecoder::on_bitsRect(Common::SeekableReadStream &stream) {
	// Copy unpacked data with clipped rectangle (8bpp or lower)
	unpackBitsRectOrRgn(stream, false, false);
}

void PICTDecoder::on_packBitsRect(Common::SeekableReadStream &stream) {
	// Unpack data (8bpp or lower)
	unpackBitsRectOrRgn(stream, true, false);
}

void PICTDecoder::on_directBitsRect(Common::SeekableReadStream &stream) {
	// Unpack data (16bpp or higher)
	PixMap pixMap = readRowBytes(stream, true);
	pixMap.rowBytes = pixMap.rowBytes & 0x7fff;
	unpackBitsRect(stream, false, pixMap);
}

void PICTDecoder::unpackBitsRectOrRgn(Common::SeekableReadStream &stream, bool compressed, bool hasRegion) {
    PixMap pixMap = readRowBytes(stream, false);
    bool hasPixMap = (pixMap.rowBytes & 0x8000);
    pixMap.rowBytes = pixMap.rowBytes & 0x7fff;

    if (hasPixMap)
        unpackBitsRect(stream, true, pixMap);
    else
        unpackBits(stream, compressed, hasRegion);
}

void PICTDecoder::on_compressedQuickTime(Common::SeekableReadStream &stream) {
	// OK, here's the fun. We get to completely change how QuickDraw draws
	// the data in PICT files.

	// Swap out the opcodes to the new ones
	_opcodes.clear();
	setupOpcodesQuickTime();

	// We'll decode the first QuickTime data from here, but the QuickTime-specific
	// opcodes will take over from here on out. Normal opcodes, signing off.
	decodeCompressedQuickTime(stream);
}

void PICTDecoder::oq_packBitsRect(Common::SeekableReadStream &stream) {
	// Skip any data here (8bpp or lower)
	skipBitsRect(stream, true);
}

void PICTDecoder::oq_directBitsRect(Common::SeekableReadStream &stream) {
	// Skip any data here (16bpp or higher)
	skipBitsRect(stream, false);
}

void PICTDecoder::oq_compressedQuickTime(Common::SeekableReadStream &stream) {
	// Just pass the data along
	decodeCompressedQuickTime(stream);
}

bool PICTDecoder::loadStream(Common::SeekableReadStream &stream) {
	destroy();

	// Initialize opcodes to their normal state
	_opcodes.clear();
	setupOpcodesNormal();

	_continueParsing = true;
	_palette.clear();

	uint16 fileSize = stream.readUint16BE();

	// If we have no file size here, we probably have a PICT from a file
	// and not a resource. The other two bytes are the fileSize which we
	// don't actually need (and already read if from a resource).
	if (!fileSize)
		stream.seek(512 + 2);

	_imageRect.top = stream.readUint16BE();
	_imageRect.left = stream.readUint16BE();
	_imageRect.bottom = stream.readUint16BE();
	_imageRect.right = stream.readUint16BE();
	_imageRect.debugPrintC(8, kDebugLevelGGraphics, "PICTDecoder::loadStream(): loaded rect: ");

	// NOTE: This is only a subset of the full PICT format.
	//     - Only V2 (Extended) Images Supported
	//     - CompressedQuickTime compressed data is supported
	//     - DirectBitsRect/PackBitsRect compressed data is supported
	for (uint32 opNum = 0; !stream.eos() && !stream.err() && stream.pos() < stream.size() && _continueParsing; opNum++) {
		// PICT v2 opcodes are two bytes
		uint16 opcode;

		if (_version != 1)
			opcode = stream.readUint16BE();
		else
			opcode = stream.readByte();

		if (opNum == 0 && (opcode != 0x0011 && opcode != 0x1101)) {
			warning("Cannot find PICT version opcode");
			return false;
		} else if (opNum == 1 && _version == 2 && opcode != 0x0C00) {
			warning("Cannot find PICT header opcode");
			return false;
		}

		// Since opcodes are word-aligned, we need to mark our starting
		// position here.
		uint32 startPos = stream.pos();

		for (uint32 i = 0; i < _opcodes.size(); i++) {
			if (_opcodes[i].op == opcode) {
				debugC(4, kDebugLevelGGraphics, "Running PICT opcode %04x '%s'", opcode, _opcodes[i].desc);
				(this->*(_opcodes[i].proc))(stream);
				break;
			} else if (i == _opcodes.size() - 1) {
				// Unknown opcode; attempt to continue forward
				warning("Unknown PICT opcode %04x", opcode);
			}
		}

		// Align
		if (_version == 2)
			stream.skip((stream.pos() - startPos) & 1);
	}

	return _outputSurface;
}

PICTDecoder::PixMap PICTDecoder::readPixMap(Common::SeekableReadStream &stream, bool hasBaseAddr, bool hasRowBytes) {
	PixMap pixMap;

	if (hasRowBytes) {
		pixMap.baseAddr = hasBaseAddr ? stream.readUint32BE() : 0;
		uint16 rowBytes = stream.readUint16BE();
		pixMap.rowBytes = rowBytes & 0x7fff;
	}

	pixMap.bounds.top = stream.readUint16BE();
	pixMap.bounds.left = stream.readUint16BE();
	pixMap.bounds.bottom = stream.readUint16BE();
	pixMap.bounds.right = stream.readUint16BE();
	pixMap.pmVersion = stream.readUint16BE();
	pixMap.packType = stream.readUint16BE();
	pixMap.packSize = stream.readUint32BE();
	pixMap.hRes = stream.readUint32BE();
	pixMap.vRes = stream.readUint32BE();
	pixMap.pixelType = stream.readUint16BE();
	pixMap.pixelSize = stream.readUint16BE();
	pixMap.cmpCount = stream.readUint16BE();
	pixMap.cmpSize = stream.readUint16BE();
	pixMap.planeBytes = stream.readUint32BE();
	pixMap.pmTable = stream.readUint32BE();
	pixMap.pmReserved = stream.readUint32BE();
	return pixMap;
}

PICTDecoder::PixMap PICTDecoder::readRowBytes(Common::SeekableReadStream &stream, bool hasBaseAddr) {
	PixMap pixMap;
	pixMap.baseAddr = hasBaseAddr ? stream.readUint32BE() : 0;
	uint16 rowBytes = stream.readUint16BE();
	pixMap.rowBytes = rowBytes;
	return pixMap;
}

struct PackBitsRectData {
	PICTDecoder::PixMap pixMap;
	Common::Rect srcRect;
	Common::Rect dstRect;
	uint16 mode;
};

void PICTDecoder::unpackBitsRect(Common::SeekableReadStream &stream, bool withPalette, PixMap pixMap) {
	PackBitsRectData packBitsData;

	packBitsData.pixMap = readPixMap(stream, !withPalette, false);
	packBitsData.pixMap.baseAddr = pixMap.baseAddr;
	packBitsData.pixMap.rowBytes = pixMap.rowBytes;

	// Read in the palette if there is one present
	if (withPalette) {
		// See https://developer.apple.com/library/archive/documentation/mac/QuickDraw/QuickDraw-267.html
		stream.readUint32BE(); // seed
		stream.readUint16BE(); // flags
		uint16 paletteColorCount = stream.readUint16BE() + 1;

		_palette.resize(paletteColorCount, false);
		for (uint16 i = 0; i < paletteColorCount; i++) {
			stream.readUint16BE();
			byte r = stream.readUint16BE() >> 8;
			byte g = stream.readUint16BE() >> 8;
			byte b = stream.readUint16BE() >> 8;
			_palette.set(i, r, g, b);
		}
	}

	packBitsData.srcRect.top = stream.readUint16BE();
	packBitsData.srcRect.left = stream.readUint16BE();
	packBitsData.srcRect.bottom = stream.readUint16BE();
	packBitsData.srcRect.right = stream.readUint16BE();
	packBitsData.dstRect.top = stream.readUint16BE();
	packBitsData.dstRect.left = stream.readUint16BE();
	packBitsData.dstRect.bottom = stream.readUint16BE();
	packBitsData.dstRect.right = stream.readUint16BE();
	packBitsData.mode = stream.readUint16BE();

	uint16 width = packBitsData.srcRect.width();
	uint16 height = packBitsData.srcRect.height();

	byte bytesPerPixel = 0;

	if (packBitsData.pixMap.pixelSize <= 8)
		bytesPerPixel = 1;
	else if (packBitsData.pixMap.pixelSize == 32)
		bytesPerPixel = packBitsData.pixMap.cmpCount;
	else
		bytesPerPixel = packBitsData.pixMap.pixelSize / 8;

	// Ensure we have enough space in the buffer to hold an entire line's worth of pixels
	uint32 lineSize = MAX<int>(width * bytesPerPixel + (8 * 2 / packBitsData.pixMap.pixelSize), packBitsData.pixMap.rowBytes);
	byte *buffer = new byte[lineSize * height]();

	// Read in amount of data per row
	for (uint16 i = 0; i < packBitsData.pixMap.bounds.height(); i++) {
		// NOTE: Compression 0 is "default". The format in SCI games is packed when 0
		// unless rowBytes is less than 8 in which case the pict can't be packed,
		// such as the shovel inventory icon in FPFP Mac. (bug #7059)
		// In the future, we may need to have something to set the "default" packing
		// format, but this is good for now.

		if (packBitsData.pixMap.packType == 1 || packBitsData.pixMap.rowBytes < 8) { // Unpacked, Pad-Byte (on 24-bit)
			// only support 1 bpp for now as there is currently only one known
			//  SCI pict that requires any unpacked support.
			if (bytesPerPixel == 1 && packBitsData.pixMap.pixelSize == 8) {
				stream.read(&buffer[i * width], width);
				if (width < packBitsData.pixMap.rowBytes) {
					// skip padding and/or clipped bytes
					stream.seek(packBitsData.pixMap.rowBytes - width, SEEK_CUR);
				}
			} else {
				// TODO: Finish this. Hasn't been needed (yet).
				error("Unpacked DirectBitsRect data (padded) with bytes per pixel: %d and pixel size: %d", bytesPerPixel, packBitsData.pixMap.pixelSize);
			}
		} else if (packBitsData.pixMap.packType == 2) { // Unpacked, No Pad-Byte (on 24-bit)
			// TODO: Finish this. Hasn't been needed (yet).
			error("Unpacked DirectBitsRect data (not padded)");
		} else if (packBitsData.pixMap.packType == 0 || packBitsData.pixMap.packType > 2) { // Packed
			uint16 byteCount = (packBitsData.pixMap.rowBytes > 250) ? stream.readUint16BE() : stream.readByte();
			unpackBitsLine(buffer + i * width * bytesPerPixel, packBitsData.pixMap.rowBytes, stream.readStream(byteCount), packBitsData.pixMap.pixelSize, bytesPerPixel);
		}
	}

	_outputSurface = new Graphics::Surface();

	switch (bytesPerPixel) {
	case 1:
		// Just copy to the image
		_outputSurface->create(width, height, Graphics::PixelFormat::createFormatCLUT8());
		memcpy(_outputSurface->getPixels(), buffer, _outputSurface->w * _outputSurface->h);
		break;
	case 2:
		// We have a 16-bit surface
		_outputSurface->create(width, height, Graphics::PixelFormat(2, 5, 5, 5, 0, 10, 5, 0, 0));
		for (uint16 y = 0; y < _outputSurface->h; y++)
			for (uint16 x = 0; x < _outputSurface->w; x++)
				WRITE_UINT16(_outputSurface->getBasePtr(x, y), READ_UINT16(buffer + (y * _outputSurface->w + x) * 2));
		break;
	case 3:
		// We have a planar 24-bit surface
		_outputSurface->create(width, height, Graphics::PixelFormat::createFormatRGB24());
		for (uint16 y = 0; y < _outputSurface->h; y++) {
			byte *dst = (byte *)_outputSurface->getBasePtr(0, y);
			for (uint16 x = 0; x < _outputSurface->w; x++) {
				*dst++ = *(buffer + y * _outputSurface->w * 3 + _outputSurface->w * 0 + x);
				*dst++ = *(buffer + y * _outputSurface->w * 3 + _outputSurface->w * 1 + x);
				*dst++ = *(buffer + y * _outputSurface->w * 3 + _outputSurface->w * 2 + x);
			}
		}
		break;
	case 4:
		// We have a planar 32-bit surface
		// Note that we ignore the alpha channel since it seems to not be correct
		// macOS does not ignore it, but then displays it incorrectly. Photoshop
		// does ignore it and displays it correctly.
		_outputSurface->create(width, height, Graphics::PixelFormat::createFormatRGB24());
		for (uint16 y = 0; y < _outputSurface->h; y++) {
			byte *dst = (byte *)_outputSurface->getBasePtr(0, y);
			for (uint16 x = 0; x < _outputSurface->w; x++) {
				// *dst++ = *(buffer + y * _outputSurface->w * 4 + _outputSurface->w * 0 + x);
				*dst++ = *(buffer + y * _outputSurface->w * 4 + _outputSurface->w * 1 + x);
				*dst++ = *(buffer + y * _outputSurface->w * 4 + _outputSurface->w * 2 + x);
				*dst++ = *(buffer + y * _outputSurface->w * 4 + _outputSurface->w * 3 + x);
			}
		}
		break;
	default:
		break;
	}

	delete[] buffer;
}

// TODO: It should be possible to merge this with unpackBitsRect, but that's
// a story for another day because this works for now.

void PICTDecoder::unpackBits(Common::SeekableReadStream &stream, bool compressed, bool hasRegion) {
	if (!_outputSurface) {
		_outputSurface = new Graphics::Surface();
		_outputSurface->create(_imageRect.width(), _imageRect.height(), Graphics::PixelFormat::createFormatCLUT8());
	}

	int y1 = stream.readSint16BE();
	int x1 = stream.readSint16BE();
	int y2 = stream.readSint16BE();
	int x2 = stream.readSint16BE();

	stream.skip(8); // srcRect
	stream.skip(8); // dstRect
	stream.skip(2); // mode

	if (hasRegion)
		stream.skip(stream.readUint16BE() - 2);

	Common::Rect outputRect(_outputSurface->w, _outputSurface->h);

	if (!compressed) {
		Common::BitStream8MSB bs(stream);

		for (int y = y1; y < y2; y++) {
			int yPos = y - _imageRect.top;

			for (int x = x1; x < x2; x++) {
				int xPos = x - _imageRect.left;

				uint bit = bs.getBit();

				if (outputRect.contains(xPos, yPos))
					_outputSurface->setPixel(xPos, yPos, bit);
			}
		}

		return;
	}

	for (int y = y1; y < y2; y++) {
		int yPos = y - _imageRect.top;
		int x = x1;

		byte rowBytes = stream.readByte();
		byte readBytes = 0;

		while (readBytes < rowBytes) {
			byte rowBuf[128];
			byte bufLen;

			byte value = stream.readByte();
			readBytes++;

			if (value >= 128) {
				bufLen = (256 - value) + 1;
				byte repeatValue = stream.readByte();
				memset(rowBuf, repeatValue, bufLen);
				readBytes++;
			} else {
				bufLen = value + 1;
				stream.read(rowBuf, bufLen);
				readBytes += bufLen;
			}

			Common::MemoryReadStream ms(rowBuf, bufLen);
			Common::BitStream8MSB bs(ms);

			for (int i = 0; i < 8 * bufLen; i++) {
				int xPos = x - _imageRect.left;
				uint bit = bs.getBit();

				if (outputRect.contains(xPos, yPos))
					_outputSurface->setPixel(xPos, yPos, bit);

				x++;
			}
		}
	}
}

void PICTDecoder::unpackBitsLine(byte *out, uint32 length, Common::SeekableReadStream *data, byte bitsPerPixel, byte bytesPerPixel) {
	uint32 dataDecoded = 0;
	byte bytesPerDecode = (bytesPerPixel == 2) ? 2 : 1;

	while (data->pos() < data->size() && dataDecoded < length) {
		byte op = data->readByte();

		if (op & 0x80) {
			uint32 runSize = (op ^ 255) + 2;
			uint16 value = (bytesPerDecode == 2) ? data->readUint16BE() : data->readByte();

			for (uint32 i = 0; i < runSize; i++) {
				if (bytesPerDecode == 2) {
					WRITE_UINT16(out, value);
					out += 2;
				} else {
					outputPixelBuffer(out, value, bitsPerPixel);
				}
			}
			dataDecoded += runSize * bytesPerDecode;
		} else {
			uint32 runSize = op + 1;

			if (bytesPerDecode == 1) {
				for (uint32 i = 0; i < runSize; i++)
					outputPixelBuffer(out, data->readByte(), bitsPerPixel);
			} else {
				for (uint32 i = 0; i < runSize; i++) {
					WRITE_UINT16(out, data->readUint16BE());
					out += 2;
				}
			}

			dataDecoded += runSize * bytesPerDecode;
		}
	}

	// HACK: Even if the data is 24-bit, rowBytes is still 32-bit
	if (bytesPerPixel == 3)
		dataDecoded += length / 4;

	if (length != dataDecoded)
		warning("Mismatched PackBits read (%d/%d)", dataDecoded, length);

	delete data;
}

void PICTDecoder::outputPixelBuffer(byte *&out, byte value, byte bitsPerPixel) {
	switch (bitsPerPixel) {
	case 1:
		for (int i = 7; i >= 0; i--)
			*out++ = (value >> i) & 1;
		break;
	case 2:
		for (int i = 6; i >= 0; i -= 2)
			*out++ = (value >> i) & 3;
		break;
	case 4:
		*out++ = (value >> 4) & 0xf;
		*out++ = value & 0xf;
		break;
	default:
		*out++ = value;
	}
}

void PICTDecoder::skipBitsRect(Common::SeekableReadStream &stream, bool withPalette) {
	// Step through a PackBitsRect/DirectBitsRect function

	if (!withPalette)
		stream.readUint32BE();

	uint16 rowBytes = stream.readUint16BE();
	uint16 height = stream.readUint16BE();
	stream.readUint16BE();
	height = stream.readUint16BE() - height;
	stream.readUint16BE();

	uint16 packType;

	// Top two bits signify PixMap vs BitMap
	if (rowBytes & 0xC000) {
		// PixMap
		stream.readUint16BE();
		packType = stream.readUint16BE();
		stream.skip(14);
		stream.readUint16BE(); // pixelSize
		stream.skip(16);

		if (withPalette) {
			stream.readUint32BE();
			stream.readUint16BE();
			stream.skip((stream.readUint16BE() + 1) * 8);
		}

		rowBytes &= 0x7FFF;
	} else {
		// BitMap
		packType = 0;
	}

	stream.skip(18);

	for (uint16 i = 0; i < height; i++) {
		if (packType == 1 || packType == 2 || rowBytes < 8)
			error("Unpacked PackBitsRect data");
		else if (packType == 0 || packType > 2)
			stream.skip((rowBytes > 250) ? stream.readUint16BE() : stream.readByte());
	}
}

// Compressed QuickTime details can be found here:
// https://developer.apple.com/library/archive/documentation/QuickTime/RM/CompressDecompress/ImageComprMgr/B-Chapter/2TheImageCompression.html
// https://developer.apple.com/library/archive/documentation/QuickTime/RM/CompressDecompress/ImageComprMgr/F-Chapter/6WorkingwiththeImage.html
void PICTDecoder::decodeCompressedQuickTime(Common::SeekableReadStream &stream) {
	// First, read all the fields from the opcode
	uint32 dataSize = stream.readUint32BE();
	uint32 startPos = stream.pos();

	/* uint16 version = */ stream.readUint16BE();

	// Read in the display matrix
	uint32 matrix[3][3];
	for (uint32 i = 0; i < 3; i++)
		for (uint32 j = 0; j < 3; j++)
			matrix[i][j] = stream.readUint32BE();

	// We currently only support offseting images vertically from the matrix
	uint16 xOffset = 0;
	uint16 yOffset = matrix[2][1] >> 16;

	uint32 matteSize = stream.readUint32BE();
	stream.skip(8); // matte rect
	/* uint16 transferMode = */ stream.readUint16BE();
	stream.skip(8); // src rect
	/* uint32 accuracy = */ stream.readUint32BE();
	uint32 maskSize = stream.readUint32BE();

	// Skip the matte and mask
	stream.skip(matteSize + maskSize);

	// Now we've reached the image descriptor, so read the relevant data from that
	uint32 idStart = stream.pos();
	uint32 idSize = stream.readUint32BE();
	uint32 codecTag = stream.readUint32BE();
	stream.skip(24); // miscellaneous stuff
	uint16 width = stream.readUint16BE();
	uint16 height = stream.readUint16BE();
	stream.skip(8); // resolution, dpi
	uint32 imageSize = stream.readUint32BE();
	stream.skip(34);
	uint16 bitsPerPixel = stream.readUint16BE();
	stream.skip(idSize - (stream.pos() - idStart)); // more useless stuff

	Common::SeekableSubReadStream imageStream(&stream, stream.pos(), stream.pos() + imageSize);

	Codec *codec = createQuickTimeCodec(codecTag, width, height, bitsPerPixel);
	if (!codec)
		error("Unhandled CompressedQuickTime format");

	const Graphics::Surface *surface = codec->decodeFrame(imageStream);

	if (!surface)
		error("PICTDecoder::decodeCompressedQuickTime(): Could not decode data");

	if (!_outputSurface) {
		_outputSurface = new Graphics::Surface();
		_outputSurface->create(_imageRect.width(), _imageRect.height(), surface->format);
	}
	assert(_outputSurface->format == surface->format);

	Common::Rect outputRect(surface->w, surface->h);
	outputRect.translate(xOffset, yOffset);
	outputRect.clip(_imageRect);

	for (uint16 y = 0; y < outputRect.height(); y++)
		memcpy(_outputSurface->getBasePtr(outputRect.left, y + outputRect.top), surface->getBasePtr(0, y), outputRect.width() * surface->format.bytesPerPixel);

	stream.seek(startPos + dataSize);
	delete codec;
}

} // End of namespace Image
