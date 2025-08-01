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

// QuickTime RLE Decoder
// Based off ffmpeg's QuickTime RLE decoder (written by Mike Melanson)

#include "image/codecs/qtrle.h"
#include "image/codecs/dither.h"

#include "common/debug.h"
#include "common/scummsys.h"
#include "common/stream.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "graphics/surface.h"

namespace Image {

QTRLEDecoder::QTRLEDecoder(uint16 width, uint16 height, byte bitsPerPixel) : Codec(), _ditherPalette(0) {
	_bitsPerPixel = bitsPerPixel;
	_width = width;
	_height = height;
	_surface = 0;
	_dirtyPalette = false;
	_colorMap = 0;

	// We need to ensure the width is a multiple of 4
	_paddedWidth = width;
	uint16 wMod = width % 4;
	if (wMod != 0)
		_paddedWidth += 4 - wMod;
}

QTRLEDecoder::~QTRLEDecoder() {
	if (_surface) {
		_surface->free();
		delete _surface;
	}

	delete[] _colorMap;
}

#define CHECK_STREAM_PTR(n) \
	do { \
		if ((stream.pos() + n) > stream.size()) { \
			warning("QTRLE Problem: stream out of bounds (%d > %d)", (int)stream.pos() + n, (int)stream.size()); \
			return; \
		} \
	} while (0)

#define CHECK_PIXEL_PTR(n) \
	do { \
		if ((int32)pixelPtr + n > (int)_paddedWidth * _surface->h) { \
			warning("QTRLE Problem: pixel ptr = %d, pixel limit = %d", pixelPtr + n, _paddedWidth * _surface->h); \
			return; \
		} \
	} while (0)

void QTRLEDecoder::decode1(Common::SeekableReadStream &stream, uint32 rowPtr, uint32 linesToChange) {
	uint32 pixelPtr = 0;
	byte *rgb = (byte *)_surface->getPixels();

	while (linesToChange) {
		CHECK_STREAM_PTR(2);
		byte skip = stream.readByte();
		int rleCode = stream.readSByte();

		if (rleCode == 0)
			break;

		if (skip & 0x80) {
			linesToChange--;
			rowPtr += _paddedWidth;
			pixelPtr = rowPtr + 2 * (skip & 0x7f);
		} else
			pixelPtr += 2 * skip;

		if (rleCode < 0) {
			// decode the run length code
			rleCode = -rleCode;
			// get the next 2 bytes from the stream, treat them as groups of 8 pixels, and output them rleCode times */
			CHECK_STREAM_PTR(2);
			byte pi0 = stream.readByte();
			byte pi1 = stream.readByte();
			CHECK_PIXEL_PTR(rleCode * 2);

			while (rleCode--) {
				rgb[pixelPtr++] = pi0;
				rgb[pixelPtr++] = pi1;
			}
		} else {
			// copy the same pixel directly to output 2 times
			rleCode *= 2;
			CHECK_STREAM_PTR(rleCode);
			CHECK_PIXEL_PTR(rleCode);

			while (rleCode--)
				rgb[pixelPtr++] = stream.readByte();
		}
	}
}

void QTRLEDecoder::decode2_4(Common::SeekableReadStream &stream, uint32 rowPtr, uint32 linesToChange, byte bpp) {
	uint32 pixelPtr = 0;
	byte *rgb = (byte *)_surface->getPixels();
	byte numPixels = (bpp == 4) ? 8 : 16;

	while (linesToChange--) {
		CHECK_STREAM_PTR(2);
		pixelPtr = rowPtr + (numPixels * (stream.readByte() - 1));

		for (int rleCode = stream.readSByte(); rleCode != -1; rleCode = stream.readSByte()) {
			if (rleCode == 0) {
				// there's another skip code in the stream
				CHECK_STREAM_PTR(1);
				pixelPtr += (numPixels * (stream.readByte() - 1));
			} else if (rleCode < 0) {
				// decode the run length code
				rleCode = -rleCode;

				// get the next 4 bytes from the stream, treat them as palette indices, and output them rleCode times */
				CHECK_STREAM_PTR(4);

				byte pi[16]; // 16 palette indices

				for (int8 i = numPixels - 1; i >= 0; i--) {
					pi[numPixels - 1 - i] = (stream.readByte() >> ((i * bpp) & 0x07)) & ((1 << bpp) - 1);

					if ((i & ((numPixels >> 2) - 1)) == 0)
						stream.readByte();
				}

				CHECK_PIXEL_PTR(rleCode * numPixels);

				while (rleCode--)
					for (byte i = 0; i < numPixels; i++)
						rgb[pixelPtr++] = pi[i];
			} else {
				// copy the same pixel directly to output 4 times
				rleCode *= 4;
				CHECK_STREAM_PTR(rleCode);
				CHECK_PIXEL_PTR(rleCode * (numPixels >> 2));

				while (rleCode--) {
					byte temp = stream.readByte();
					if (bpp == 4) {
						rgb[pixelPtr++] = (temp >> 4) & 0x0f;
						rgb[pixelPtr++] = temp & 0x0f;
					} else {
						rgb[pixelPtr++] = (temp >> 6) & 0x03;
						rgb[pixelPtr++] = (temp >> 4) & 0x03;
						rgb[pixelPtr++] = (temp >> 2) & 0x03;
						rgb[pixelPtr++] = temp & 0x03;
					}
				}
			}
		}

		rowPtr += _paddedWidth;
	}
}

void QTRLEDecoder::decode8(Common::SeekableReadStream &stream, uint32 rowPtr, uint32 linesToChange) {
	uint32 pixelPtr = 0;
	byte *rgb = (byte *)_surface->getPixels();

	while (linesToChange--) {
		CHECK_STREAM_PTR(2);
		pixelPtr = rowPtr + 4 * (stream.readByte() - 1);

		for (int rleCode = stream.readSByte(); rleCode != -1; rleCode = stream.readSByte()) {
			if (rleCode == 0) {
				// there's another skip code in the stream
				CHECK_STREAM_PTR(1);
				pixelPtr += 4 * (stream.readByte() - 1);
			} else if (rleCode < 0) {
				// decode the run length code
				rleCode = -rleCode;

				// get the next 4 bytes from the stream, treat them as palette indices, and output them rleCode times
				CHECK_STREAM_PTR(4);

				byte pi[4];  // 4 palette indexes

				for (byte i = 0; i < 4; i++)
					pi[i] = stream.readByte();

				CHECK_PIXEL_PTR(rleCode * 4);

				while (rleCode--)
					for (byte i = 0; i < 4; i++)
						rgb[pixelPtr++] = pi[i];
			} else {
				// copy the same pixel directly to output 4 times
				rleCode *= 4;
				CHECK_STREAM_PTR(rleCode);
				CHECK_PIXEL_PTR(rleCode);

				while (rleCode--)
					rgb[pixelPtr++] = stream.readByte();
			}
		}

		rowPtr += _paddedWidth;
	}
}

void QTRLEDecoder::decode16(Common::SeekableReadStream &stream, uint32 rowPtr, uint32 linesToChange) {
	uint32 pixelPtr = 0;
	uint16 *rgb = (uint16 *)_surface->getPixels();

	while (linesToChange--) {
		CHECK_STREAM_PTR(2);
		pixelPtr = rowPtr + stream.readByte() - 1;

		for (int rleCode = stream.readSByte(); rleCode != -1; rleCode = stream.readSByte()) {
			if (rleCode == 0) {
				// there's another skip code in the stream
				CHECK_STREAM_PTR(1);
				pixelPtr += stream.readByte() - 1;
			} else if (rleCode < 0) {
				// decode the run length code
				rleCode = -rleCode;
				CHECK_STREAM_PTR(2);

				uint16 rgb16 = stream.readUint16BE();

				CHECK_PIXEL_PTR(rleCode);

				while (rleCode--)
					rgb[pixelPtr++] = rgb16;
			} else {
				CHECK_STREAM_PTR(rleCode * 2);
				CHECK_PIXEL_PTR(rleCode);

				// copy pixels directly to output
				while (rleCode--)
					rgb[pixelPtr++] = stream.readUint16BE();
			}
		}

		rowPtr += _paddedWidth;
	}
}

namespace {

inline uint16 readDitherColor16(Common::ReadStream &stream) {
	return stream.readUint16BE() >> 1;
}

} // End of anonymous namespace

void QTRLEDecoder::dither16(Common::SeekableReadStream &stream, uint32 rowPtr, uint32 linesToChange) {
	uint32 pixelPtr = 0;
	byte *output = (byte *)_surface->getPixels();

	static const uint16 colorTableOffsets[] = { 0x0000, 0xC000, 0x4000, 0x8000 };

	// clone2727 thinks this should be startLine & 3, but the original definitely
	// isn't doing this. Unless startLine & 3 is always 0? Kinda defeats the
	// purpose of the compression then.
	byte curColorTableOffset = 0;

	while (linesToChange--) {
		CHECK_STREAM_PTR(2);

		byte rowOffset = stream.readByte() - 1;
		pixelPtr = rowPtr + rowOffset;
		uint16 colorTableOffset = colorTableOffsets[curColorTableOffset] + (rowOffset << 14);

		for (int rleCode = stream.readSByte(); rleCode != -1; rleCode = stream.readSByte()) {
			if (rleCode == 0) {
				// there's another skip code in the stream
				CHECK_STREAM_PTR(1);
				pixelPtr += stream.readByte() - 1;
			} else if (rleCode < 0) {
				// decode the run length code
				rleCode = -rleCode;
				CHECK_STREAM_PTR(2);

				uint16 color = readDitherColor16(stream);

				CHECK_PIXEL_PTR(rleCode);

				while (rleCode--) {
					output[pixelPtr++] = _colorMap[colorTableOffset + color];
					colorTableOffset += 0x4000;
				}
			} else {
				CHECK_STREAM_PTR(rleCode * 2);
				CHECK_PIXEL_PTR(rleCode);

				// copy pixels directly to output
				while (rleCode--) {
					uint16 color = readDitherColor16(stream);
					output[pixelPtr++] = _colorMap[colorTableOffset + color];
					colorTableOffset += 0x4000;
				}
			}
		}

		rowPtr += _paddedWidth;
		curColorTableOffset = (curColorTableOffset + 1) & 3;
	}
}

void QTRLEDecoder::decode24(Common::SeekableReadStream &stream, uint32 rowPtr, uint32 linesToChange) {
	uint32 pixelPtr = 0;
	uint8 *rgb = (uint8 *)_surface->getPixels();

	while (linesToChange--) {
		CHECK_STREAM_PTR(2);
		pixelPtr = rowPtr + stream.readByte() - 1;

		for (int rleCode = stream.readSByte(); rleCode != -1; rleCode = stream.readSByte()) {
			if (rleCode == 0) {
				// there's another skip code in the stream
				CHECK_STREAM_PTR(1);
				pixelPtr += stream.readByte() - 1;
			} else if (rleCode < 0) {
				// decode the run length code
				rleCode = -rleCode;

				CHECK_STREAM_PTR(3);

				byte r = stream.readByte();
				byte g = stream.readByte();
				byte b = stream.readByte();

				CHECK_PIXEL_PTR(rleCode);

				while (rleCode--) {
					rgb[(pixelPtr * 3) + 0] = r;
					rgb[(pixelPtr * 3) + 1] = g;
					rgb[(pixelPtr * 3) + 2] = b;
					pixelPtr++;
				}
			} else {
				CHECK_STREAM_PTR(rleCode * 3);
				CHECK_PIXEL_PTR(rleCode);

				// copy pixels directly to output
				stream.read(&rgb[pixelPtr * 3], rleCode * 3);
				pixelPtr += rleCode;
			}
		}

		rowPtr += _paddedWidth;
	}
}

namespace {

inline uint16 readDitherColor24(Common::ReadStream &stream) {
	uint8 rgb[3];
	stream.read(rgb, 3);

	uint16 color = (rgb[0] & 0xF8) << 6;
	color |= (rgb[1] & 0xF8) << 1;
	color |= rgb[2] >> 4;
	return color;
}

} // End of anonymous namespace

void QTRLEDecoder::dither24(Common::SeekableReadStream &stream, uint32 rowPtr, uint32 linesToChange) {
	uint32 pixelPtr = 0;
	byte *output = (byte *)_surface->getPixels();

	static const uint16 colorTableOffsets[] = { 0x0000, 0xC000, 0x4000, 0x8000 };

	// clone2727 thinks this should be startLine & 3, but the original definitely
	// isn't doing this. Unless startLine & 3 is always 0? Kinda defeats the
	// purpose of the compression then.
	byte curColorTableOffset = 0;

	while (linesToChange--) {
		CHECK_STREAM_PTR(2);

		byte rowOffset = stream.readByte() - 1;
		pixelPtr = rowPtr + rowOffset;
		uint16 colorTableOffset = colorTableOffsets[curColorTableOffset] + (rowOffset << 14);

		for (int rleCode = stream.readSByte(); rleCode != -1; rleCode = stream.readSByte()) {
			if (rleCode == 0) {
				// there's another skip code in the stream
				CHECK_STREAM_PTR(1);
				pixelPtr += stream.readByte() - 1;
			} else if (rleCode < 0) {
				// decode the run length code
				rleCode = -rleCode;
				CHECK_STREAM_PTR(3);

				uint16 color = readDitherColor24(stream);

				CHECK_PIXEL_PTR(rleCode);

				while (rleCode--) {
					output[pixelPtr++] = _colorMap[colorTableOffset + color];
					colorTableOffset += 0x4000;
				}
			} else {
				CHECK_STREAM_PTR(rleCode * 3);
				CHECK_PIXEL_PTR(rleCode);

				// copy pixels directly to output
				while (rleCode--) {
					uint16 color = readDitherColor24(stream);
					output[pixelPtr++] = _colorMap[colorTableOffset + color];
					colorTableOffset += 0x4000;
				}
			}
		}

		rowPtr += _paddedWidth;
		curColorTableOffset = (curColorTableOffset + 1) & 3;
	}
}

void QTRLEDecoder::decode32(Common::SeekableReadStream &stream, uint32 rowPtr, uint32 linesToChange) {
	uint32 pixelPtr = 0;
	uint8 *rgb = (uint8 *)_surface->getPixels();

	while (linesToChange--) {
		CHECK_STREAM_PTR(2);
		pixelPtr = rowPtr + stream.readByte() - 1;

		for (int rleCode = stream.readSByte(); rleCode != -1; rleCode = stream.readSByte()) {
			if (rleCode == 0) {
				// there's another skip code in the stream
				CHECK_STREAM_PTR(1);
				pixelPtr += stream.readByte() - 1;
			} else if (rleCode < 0) {
				// decode the run length code
				rleCode = -rleCode;

				CHECK_STREAM_PTR(4);

				byte a = stream.readByte();
				byte r = stream.readByte();
				byte g = stream.readByte();
				byte b = stream.readByte();

				CHECK_PIXEL_PTR(rleCode);

				while (rleCode--) {
					rgb[(pixelPtr * 4) + 0] = a;
					rgb[(pixelPtr * 4) + 1] = r;
					rgb[(pixelPtr * 4) + 2] = g;
					rgb[(pixelPtr * 4) + 3] = b;
					pixelPtr++;
				}
			} else {
				CHECK_STREAM_PTR(rleCode * 4);
				CHECK_PIXEL_PTR(rleCode);

				// copy pixels directly to output
				stream.read(&rgb[pixelPtr * 4], rleCode * 4);
				pixelPtr += rleCode;
			}
		}

		rowPtr += _paddedWidth;
	}
}

namespace {

inline uint16 readDitherColor32(Common::ReadStream &stream) {
	uint8 argb[4];
	stream.read(argb, 4);

	uint16 color = (argb[1] & 0xF8) << 6;
	color |= (argb[2] & 0xF8) << 1;
	color |= argb[3] >> 4;
	return color;
}

} // End of anonymous namespace

void QTRLEDecoder::dither32(Common::SeekableReadStream &stream, uint32 rowPtr, uint32 linesToChange) {
	uint32 pixelPtr = 0;
	byte *output = (byte *)_surface->getPixels();

	static const uint16 colorTableOffsets[] = { 0x0000, 0xC000, 0x4000, 0x8000 };

	// clone2727 thinks this should be startLine & 3, but the original definitely
	// isn't doing this. Unless startLine & 3 is always 0? Kinda defeats the
	// purpose of the compression then.
	byte curColorTableOffset = 0;

	while (linesToChange--) {
		CHECK_STREAM_PTR(2);

		byte rowOffset = stream.readByte() - 1;
		pixelPtr = rowPtr + rowOffset;
		uint16 colorTableOffset = colorTableOffsets[curColorTableOffset] + (rowOffset << 14);

		for (int rleCode = stream.readSByte(); rleCode != -1; rleCode = stream.readSByte()) {
			if (rleCode == 0) {
				// there's another skip code in the stream
				CHECK_STREAM_PTR(1);
				pixelPtr += stream.readByte() - 1;
			} else if (rleCode < 0) {
				// decode the run length code
				rleCode = -rleCode;
				CHECK_STREAM_PTR(4);

				uint16 color = readDitherColor32(stream);

				CHECK_PIXEL_PTR(rleCode);

				while (rleCode--) {
					output[pixelPtr++] = _colorMap[colorTableOffset + color];
					colorTableOffset += 0x4000;
				}
			} else {
				CHECK_STREAM_PTR(rleCode * 4);
				CHECK_PIXEL_PTR(rleCode);

				// copy pixels directly to output
				while (rleCode--) {
					uint16 color = readDitherColor32(stream);
					output[pixelPtr++] = _colorMap[colorTableOffset + color];
					colorTableOffset += 0x4000;
				}
			}
		}

		rowPtr += _paddedWidth;
		curColorTableOffset = (curColorTableOffset + 1) & 3;
	}
}

const Graphics::Surface *QTRLEDecoder::decodeFrame(Common::SeekableReadStream &stream) {
	if (!_surface)
		createSurface();

	uint16 startLine = 0;
	uint16 height = _height;

	// check if this frame is even supposed to change
	if (stream.size() < 8)
		return _surface;

	// start after the chunk size
	stream.readUint32BE();

	// fetch the header
	uint16 header = stream.readUint16BE();

	// if a header is present, fetch additional decoding parameters
	if (header & 8) {
		if (stream.size() < 14)
			return _surface;

		startLine = stream.readUint16BE();
		stream.readUint16BE(); // Unknown
		height = stream.readUint16BE();
		stream.readUint16BE(); // Unknown
	}

	uint32 rowPtr = _paddedWidth * startLine;

	switch (_bitsPerPixel) {
	case 1:
	case 33:
		decode1(stream, rowPtr, height);
		break;
	case 2:
	case 34:
		decode2_4(stream, rowPtr, height, 2);
		break;
	case 4:
	case 36:
		decode2_4(stream, rowPtr, height, 4);
		break;
	case 8:
	case 40:
		decode8(stream, rowPtr, height);
		break;
	case 16:
		if (_ditherPalette.size() > 0)
			dither16(stream, rowPtr, height);
		else
			decode16(stream, rowPtr, height);
		break;
	case 24:
		if (_ditherPalette.size() > 0)
			dither24(stream, rowPtr, height);
		else
			decode24(stream, rowPtr, height);
		break;
	case 32:
		if (_ditherPalette.size() > 0)
			dither32(stream, rowPtr, height);
		else
			decode32(stream, rowPtr, height);
		break;
	default:
		error("Unsupported QTRLE bits per pixel %d", _bitsPerPixel);
	}

	return _surface;
}

Graphics::PixelFormat QTRLEDecoder::getPixelFormat() const {
	if (_ditherPalette.size() > 0)
		return Graphics::PixelFormat::createFormatCLUT8();

	switch (_bitsPerPixel) {
	case 1:
	case 33:
	case 2:
	case 34:
	case 4:
	case 36:
	case 8:
	case 40:
		return Graphics::PixelFormat::createFormatCLUT8();
	case 16:
		return Graphics::PixelFormat(2, 5, 5, 5, 0, 10, 5, 0, 0);
	case 24:
		return Graphics::PixelFormat::createFormatRGB24();
	case 32:
		return Graphics::PixelFormat::createFormatARGB32();
	default:
		error("Unsupported QTRLE bits per pixel %d", _bitsPerPixel);
	}

	return Graphics::PixelFormat();
}

bool QTRLEDecoder::canDither(DitherType type) const {
	return type == kDitherTypeQT && (_bitsPerPixel == 16 || _bitsPerPixel == 24 || _bitsPerPixel == 32);
}

void QTRLEDecoder::setDither(DitherType type, const byte *palette) {
	assert(canDither(type));

	_ditherPalette.resize(256, false);
	_ditherPalette.set(palette, 0, 256);
	_dirtyPalette = true;

	delete[] _colorMap;
	_colorMap = DitherCodec::createQuickTimeDitherTable(palette, 256);
}

void QTRLEDecoder::createSurface() {
	if (_surface) {
		_surface->free();
		delete _surface;
	}

	_surface = new Graphics::Surface();
	_surface->create(_paddedWidth, _height, getPixelFormat());
	_surface->w = _width;
}

} // End of namespace Image
