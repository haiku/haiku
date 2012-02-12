/*
 * Copyright 2007-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 1997-2007, Dave Coffin, dcoffin a cybercom o net
 * This code is based on Dave Coffin's dcraw 8.63 - it's basically the same
 * thing in C++, but follows common sense programming rules a bit more :-)
 * Except the Fovean functions, dcraw is public domain.
 */


#include "RAW.h"
#include "ReadHelper.h"

#include <Message.h>
#include <TranslationErrors.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#define TRACE(x) printf x
#define TRACE(x)
//#define TAG(x...) printf(x)
#define TAG(x...)

#define ABS(x) (((int)(x) ^ ((int)(x) >> 31)) - ((int)(x) >> 31))
#define LIM(x,min,max) MAX(min,MIN(x,max))
#define ULIM(x,y,z) ((y) < (z) ? LIM(x,y,z) : LIM(x,z,y))
#define CLIP(x) LIM(x,0,65535)
#define SWAP(a,b) { a ^= b; a ^= (b ^= a); }

#define FC(row,col) \
	(fFilters >> ((((row) << 1 & 14) + ((col) & 1)) << 1) & 3)


static const uint32 kImageBufferCount = 10;
static const uint32 kDecodeBufferCount = 2048;

const double xyz_rgb[3][3] = {			/* XYZ from RGB */
  { 0.412453, 0.357580, 0.180423 },
  { 0.212671, 0.715160, 0.072169 },
  { 0.019334, 0.119193, 0.950227 } };
const float kD65White[3] = { 0.950456, 1, 1.088754 };

struct decode {
	struct decode *branch[2];
	int32	leaf;
};

struct jhead {
	int		bits, high, wide, clrs, restart, vpred[4];
	struct decode *huff[4];
	uint16*	row;
};

struct tiff_header {
	uint16	order, magic;
	int32	image_file_directory;
	uint16	pad, ntag;
	struct tiff_tag tag[15];
	int32	next_image_file_directory;
	uint16	pad2, nexif;
	struct tiff_tag exif[4];
	int16	bps[4];
	int32	rat[6];
	char make[64], model[64], soft[32], date[20];
};


template<class T> inline T
square(const T& value)
{
	return value * value;
}


static inline bool
x_flipped(int32 orientation)
{
	return orientation == 2 || orientation == 3
		|| orientation == 7 || orientation == 8;
}


static inline bool
y_flipped(int32 orientation)
{
	return orientation == 3 || orientation == 4
		|| orientation == 6 || orientation == 7;
}


#if 0
void
dump_to_disk(void* data, size_t length)
{
	FILE* file = fopen("/tmp/RAW.out", "wb");
	if (file == NULL)
		return;

	fwrite(data, length, 1, file);
	fclose(file);
}
#endif


//	#pragma mark -


DCRaw::DCRaw(BPositionIO& stream)
	:
	fRead(stream),
	fNumImages(0),
	fRawIndex(-1),
	fThumbIndex(-1),
	fDNGVersion(0),
	fIsTIFF(false),
	fImageData(NULL),
	fThreshold(0.0f),
	fHalfSize(false),
	fUseCameraWhiteBalance(true),
	fUseAutoWhiteBalance(true),
	fRawColor(true),
	fUseGamma(true),
	fBrightness(1.0f),
	fOutputColor(1),
	fHighlight(0),
	fDocumentMode(0),
	fOutputWidth(0),
	fOutputHeight(0),
	fInputWidth(0),
	fInputHeight(0),
	fTopMargin(0),
	fLeftMargin(0),
	fColors(3),
	fOutputProfile(NULL),
	fOutputBitsPerSample(8),
	fDecodeLeaf(0),
	fDecodeBitsZeroAfterMax(false),
	fFilters(~0),
	fEXIFOffset(-1),
	fProgressMonitor(NULL)
{
	fImages = new image_data_info[kImageBufferCount];
	fDecodeBuffer = new decode[kDecodeBufferCount];
	fCurve = new uint16[0x1000];
	for (uint32 i = 0; i < 0x1000; i++) {
		fCurve[i] = i;
	}

	cbrt = new float[0x10000];
	fHistogram = (int32 (*)[4])calloc(sizeof(int32) * 0x2000 * 4, 1);

	memset(fImages, 0, sizeof(image_data_info) * kImageBufferCount);
	memset(&fMeta, 0, sizeof(image_meta_info));
	memset(fUserMultipliers, 0, sizeof(fUserMultipliers));
	memset(fWhite, 0, sizeof(fWhite));

	fMeta.camera_multipliers[0] = -1;
	fCR2Slice[0] = 0;
}


DCRaw::~DCRaw()
{
	delete[] fImages;
	delete[] fDecodeBuffer;
	delete[] fCurve;

	delete[] cbrt;

	free(fHistogram);
	free(fImageData);
}


int32
DCRaw::_AllocateImage()
{
	if (fNumImages + 1 == kImageBufferCount)
		throw (status_t)B_ERROR;

	return fNumImages++;
}


image_data_info&
DCRaw::_Raw()
{
	if (fRawIndex < 0)
		fRawIndex = _AllocateImage();
	if (fRawIndex < 0)
		throw (status_t)B_ERROR;

	return fImages[fRawIndex];
}


image_data_info&
DCRaw::_Thumb()
{
	if (fThumbIndex < 0)
		fThumbIndex = _AllocateImage();
	if (fThumbIndex < 0)
		throw (status_t)B_ERROR;

	return fImages[fThumbIndex];
}


//! Make sure that the raw image always comes first
void
DCRaw::_CorrectIndex(uint32& index) const
{
	if (fRawIndex > 0) {
		if (index == 0)
			index = fRawIndex;
		else if (index <= (uint32)fRawIndex)
			index--;
	}
}


inline uint16&
DCRaw::_Bayer(int32 column, int32 row)
{
	return fImageData[((row) >> fShrink) * fOutputWidth
		+ ((column) >> fShrink)][FC(row, column)];
}


inline int32
DCRaw::_FilterCoefficient(int32 x, int32 y)
{
	static const char filter[16][16] = {
		{ 2,1,1,3,2,3,2,0,3,2,3,0,1,2,1,0 },
		{ 0,3,0,2,0,1,3,1,0,1,1,2,0,3,3,2 },
		{ 2,3,3,2,3,1,1,3,3,1,2,1,2,0,0,3 },
		{ 0,1,0,1,0,2,0,2,2,0,3,0,1,3,2,1 },
		{ 3,1,1,2,0,1,0,2,1,3,1,3,0,1,3,0 },
		{ 2,0,0,3,3,2,3,1,2,0,2,0,3,2,2,1 },
		{ 2,3,3,1,2,1,2,1,2,1,1,2,3,0,0,1 },
		{ 1,0,0,2,3,0,0,3,0,3,0,3,2,1,2,3 },
		{ 2,3,3,1,1,2,1,0,3,2,3,0,2,3,1,3 },
		{ 1,0,2,0,3,0,3,2,0,1,1,2,0,1,0,2 },
		{ 0,1,1,3,3,2,2,1,1,3,3,0,2,1,3,2 },
		{ 2,3,2,0,0,1,3,0,2,0,1,2,3,0,1,0 },
		{ 1,3,1,2,3,2,3,2,0,2,0,1,1,0,3,0 },
		{ 0,2,0,3,1,0,0,1,1,3,3,2,3,2,2,1 },
		{ 2,1,3,2,3,1,2,1,0,3,0,2,0,2,0,2 },
		{ 0,3,1,0,0,2,0,3,2,1,3,1,1,3,1,3 }
	};

	if (fFilters != 1)
		return FC(y, x);

	return filter[(y + fTopMargin) & 15][(x + fLeftMargin) & 15];
}


inline int32
DCRaw::_FlipIndex(uint32 row, uint32 col, uint32 flip)
{
	if (flip > 4)
		SWAP(row, col);
	if (y_flipped(flip))
		row = fInputHeight - 1 - row;
	if (x_flipped(flip))
		col = fInputWidth - 1 - col;

	return row * fInputWidth + col;
}


bool
DCRaw::_SupportsCompression(image_data_info& image) const
{
	switch (image.compression) {
		//case COMPRESSION_NONE:
		case COMPRESSION_OLD_JPEG:
		case COMPRESSION_PACKBITS:
			return true;

		default:
			return false;
	}
}


bool
DCRaw::_IsCanon() const
{
	return !strncasecmp(fMeta.manufacturer, "Canon", 5);
}


bool
DCRaw::_IsKodak() const
{
	return !strncasecmp(fMeta.manufacturer, "Kodak", 5);
}


bool
DCRaw::_IsNikon() const
{
	return !strncasecmp(fMeta.manufacturer, "Nikon", 5);
}


bool
DCRaw::_IsOlympus() const
{
	return !strncasecmp(fMeta.manufacturer, "Olympus", 7);
}


bool
DCRaw::_IsPentax() const
{
	return !strncasecmp(fMeta.manufacturer, "Pentax", 6);
}


bool
DCRaw::_IsSamsung() const
{
	return !strncasecmp(fMeta.manufacturer, "Samsung", 7);
}


void
DCRaw::_ParseThumbTag(off_t baseOffset, uint32 offsetTag, uint32 lengthTag)
{
	uint16 entries;
	fRead(entries);

	while (entries--) {
		off_t nextOffset;
		tiff_tag tag;
		_ParseTIFFTag(baseOffset, tag, nextOffset);

		if (tag.tag == offsetTag)
			_Thumb().data_offset = fRead.Next<uint32>();
		if (tag.tag == lengthTag)
			_Thumb().bytes = fRead.Next<uint32>();

		fRead.Seek(nextOffset, SEEK_SET);
	}
}


void
DCRaw::_ParseManufacturerTag(off_t baseOffset)
{
	static const uchar xlat[2][256] = {
		{
			0xc1,0xbf,0x6d,0x0d,0x59,0xc5,0x13,0x9d,0x83,0x61,0x6b,0x4f,0xc7,0x7f,0x3d,0x3d,
			0x53,0x59,0xe3,0xc7,0xe9,0x2f,0x95,0xa7,0x95,0x1f,0xdf,0x7f,0x2b,0x29,0xc7,0x0d,
			0xdf,0x07,0xef,0x71,0x89,0x3d,0x13,0x3d,0x3b,0x13,0xfb,0x0d,0x89,0xc1,0x65,0x1f,
			0xb3,0x0d,0x6b,0x29,0xe3,0xfb,0xef,0xa3,0x6b,0x47,0x7f,0x95,0x35,0xa7,0x47,0x4f,
			0xc7,0xf1,0x59,0x95,0x35,0x11,0x29,0x61,0xf1,0x3d,0xb3,0x2b,0x0d,0x43,0x89,0xc1,
			0x9d,0x9d,0x89,0x65,0xf1,0xe9,0xdf,0xbf,0x3d,0x7f,0x53,0x97,0xe5,0xe9,0x95,0x17,
			0x1d,0x3d,0x8b,0xfb,0xc7,0xe3,0x67,0xa7,0x07,0xf1,0x71,0xa7,0x53,0xb5,0x29,0x89,
			0xe5,0x2b,0xa7,0x17,0x29,0xe9,0x4f,0xc5,0x65,0x6d,0x6b,0xef,0x0d,0x89,0x49,0x2f,
			0xb3,0x43,0x53,0x65,0x1d,0x49,0xa3,0x13,0x89,0x59,0xef,0x6b,0xef,0x65,0x1d,0x0b,
			0x59,0x13,0xe3,0x4f,0x9d,0xb3,0x29,0x43,0x2b,0x07,0x1d,0x95,0x59,0x59,0x47,0xfb,
			0xe5,0xe9,0x61,0x47,0x2f,0x35,0x7f,0x17,0x7f,0xef,0x7f,0x95,0x95,0x71,0xd3,0xa3,
			0x0b,0x71,0xa3,0xad,0x0b,0x3b,0xb5,0xfb,0xa3,0xbf,0x4f,0x83,0x1d,0xad,0xe9,0x2f,
			0x71,0x65,0xa3,0xe5,0x07,0x35,0x3d,0x0d,0xb5,0xe9,0xe5,0x47,0x3b,0x9d,0xef,0x35,
			0xa3,0xbf,0xb3,0xdf,0x53,0xd3,0x97,0x53,0x49,0x71,0x07,0x35,0x61,0x71,0x2f,0x43,
			0x2f,0x11,0xdf,0x17,0x97,0xfb,0x95,0x3b,0x7f,0x6b,0xd3,0x25,0xbf,0xad,0xc7,0xc5,
			0xc5,0xb5,0x8b,0xef,0x2f,0xd3,0x07,0x6b,0x25,0x49,0x95,0x25,0x49,0x6d,0x71,0xc7
		},
		{
			0xa7,0xbc,0xc9,0xad,0x91,0xdf,0x85,0xe5,0xd4,0x78,0xd5,0x17,0x46,0x7c,0x29,0x4c,
			0x4d,0x03,0xe9,0x25,0x68,0x11,0x86,0xb3,0xbd,0xf7,0x6f,0x61,0x22,0xa2,0x26,0x34,
			0x2a,0xbe,0x1e,0x46,0x14,0x68,0x9d,0x44,0x18,0xc2,0x40,0xf4,0x7e,0x5f,0x1b,0xad,
			0x0b,0x94,0xb6,0x67,0xb4,0x0b,0xe1,0xea,0x95,0x9c,0x66,0xdc,0xe7,0x5d,0x6c,0x05,
			0xda,0xd5,0xdf,0x7a,0xef,0xf6,0xdb,0x1f,0x82,0x4c,0xc0,0x68,0x47,0xa1,0xbd,0xee,
			0x39,0x50,0x56,0x4a,0xdd,0xdf,0xa5,0xf8,0xc6,0xda,0xca,0x90,0xca,0x01,0x42,0x9d,
			0x8b,0x0c,0x73,0x43,0x75,0x05,0x94,0xde,0x24,0xb3,0x80,0x34,0xe5,0x2c,0xdc,0x9b,
			0x3f,0xca,0x33,0x45,0xd0,0xdb,0x5f,0xf5,0x52,0xc3,0x21,0xda,0xe2,0x22,0x72,0x6b,
			0x3e,0xd0,0x5b,0xa8,0x87,0x8c,0x06,0x5d,0x0f,0xdd,0x09,0x19,0x93,0xd0,0xb9,0xfc,
			0x8b,0x0f,0x84,0x60,0x33,0x1c,0x9b,0x45,0xf1,0xf0,0xa3,0x94,0x3a,0x12,0x77,0x33,
			0x4d,0x44,0x78,0x28,0x3c,0x9e,0xfd,0x65,0x57,0x16,0x94,0x6b,0xfb,0x59,0xd0,0xc8,
			0x22,0x36,0xdb,0xd2,0x63,0x98,0x43,0xa1,0x04,0x87,0x86,0xf7,0xa6,0x26,0xbb,0xd6,
			0x59,0x4d,0xbf,0x6a,0x2e,0xaa,0x2b,0xef,0xe6,0x78,0xb6,0x4e,0xe0,0x2f,0xdc,0x7c,
			0xbe,0x57,0x19,0x32,0x7e,0x2a,0xd0,0xb8,0xba,0x29,0x00,0x3c,0x52,0x7d,0xa8,0x49,
			0x3b,0x2d,0xeb,0x25,0x49,0xfa,0xa3,0xaa,0x39,0xa7,0xc5,0xa7,0x50,0x11,0x36,0xfb,
			0xc6,0x67,0x4a,0xf5,0xa5,0x12,0x65,0x7e,0xb0,0xdf,0xaf,0x4e,0xb3,0x61,0x7f,0x2f
		}
	};

	uint32 ver97 = 0, serial = 0;
	uchar buf97[324], ci, cj, ck;
	bool originalSwap = fRead.IsSwapping();
	image_data_info& image = fImages[fNumImages];

	// The MakerNote might have its own TIFF header (possibly with
	// its own byte-order!), or it might just be a table.

	char type[10];
	fRead(type, sizeof(type));

	if (!strncmp(type, "KDK", 3)
		|| !strncmp(type, "VER", 3)
		|| !strncmp(type, "IIII", 4)
		|| !strncmp(type, "MMMM", 4)) {
		// these aren't TIFF tables
		return;
	}
	if (!strncmp(type, "KC", 2)			// Konica KD-400Z, KD-510Z
		|| !strncmp(type, "MLY", 3)) {	// Minolta DiMAGE G series
		fRead.SetSwap(B_HOST_IS_LENDIAN != 0);
			// this chunk is always in big endian

		uint32 whiteBalance[4] = {0, 0, 0, 0};

		off_t offset;
	    while ((offset = fRead.Position()) < image.data_offset
	    	&& offset < 16384) {
			whiteBalance[0] = whiteBalance[2];
			whiteBalance[2] = whiteBalance[1];
			whiteBalance[1] = whiteBalance[3];

			whiteBalance[3] = fRead.Next<uint16>();
			if (whiteBalance[1] == 256 && whiteBalance[3] == 256
				&& whiteBalance[0] > 256 && whiteBalance[0] < 640
				&& whiteBalance[2] > 256 && whiteBalance[2] < 640) {
				for (uint32 i = 0; i < 4; i++) {
					fMeta.camera_multipliers[i] = whiteBalance[i];
				}
			}
		}
		goto quit;
	}
	if (!strcmp(type, "Nikon")) {
		baseOffset = fRead.Position();

		uint16 endian;
		fRead(endian);

#if B_HOST_IS_LENDIAN
		fRead.SetSwap(endian == 'MM');
#else
		fRead.SetSwap(endian == 'II');
#endif

		if (fRead.Next<uint16>() != 42)
			goto quit;

		uint32 offset = fRead.Next<uint32>();
		fRead.Seek(offset - 8, SEEK_CUR);
	} else if (!strncmp(type, "FUJIFILM", 8)
		|| !strncmp(type, "SONY", 4)
		|| !strcmp(type, "Panasonic")) {
		fRead.SetSwap(B_HOST_IS_BENDIAN != 0);
			// this chunk is always in little endian
		fRead.Seek(2, SEEK_CUR);
	} else if (!strcmp(type, "OLYMP")
		|| !strcmp(type, "LEICA")
		|| !strcmp(type, "Ricoh")
		|| !strcmp(type, "EPSON"))
		fRead.Seek(-2, SEEK_CUR);
	else if (!strcmp(type, "AOC") || !strcmp(type, "QVC"))
		fRead.Seek(-4, SEEK_CUR);
	else
		fRead.Seek(-10, SEEK_CUR);

	uint16 entries;
	fRead(entries);
	if (entries > 1000)
		return;

	while (entries--) {
		off_t nextOffset;
		tiff_tag tag;
		_ParseTIFFTag(baseOffset, tag, nextOffset);
		TAG("Manufacturer tag %u (type %u, length %lu)\n", tag.tag, tag.type,
			tag.length);

		if (strstr(fMeta.manufacturer, "PENTAX")) {
			if (tag.tag == 0x1b)
				tag.tag = 0x1018;
			if (tag.tag == 0x1c)
				tag.tag = 0x1017;
		} else if (tag.tag == 2 && strstr(fMeta.manufacturer, "NIKON")) {
			fRead.Next<uint16>();
				// ignored
			fMeta.iso_speed = fRead.Next<uint16>();
		}

		if (tag.tag == 4 && tag.length == 27) {
			fRead.Next<uint32>();
				// ignored
			fMeta.iso_speed = 50 * pow(2, fRead.Next<uint16>() / 32.0 - 4);
			fRead.Next<uint16>();
				// ignored
			fMeta.aperture = pow(2, fRead.Next<uint16>() / 64.0);
			fMeta.shutter = pow(2, fRead.Next<int16>() / -32.0);
		}
		if (tag.tag == 8 && tag.type == 4)
			fMeta.shot_order = fRead.Next<uint32>();
		if (tag.tag == 0xc && tag.length == 4) {
			fMeta.camera_multipliers[0] = fRead.NextDouble(TIFF_FRACTION_TYPE);
			fMeta.camera_multipliers[2] = fRead.NextDouble(TIFF_FRACTION_TYPE);
		}
		if (tag.tag == 0x10 && tag.type == 4)
			fUniqueID = fRead.Next<uint32>();
		if (tag.tag == 0x11) {
			if (_ParseTIFFImageFileDirectory(baseOffset, fRead.Next<uint32>())
					== B_OK)
				fNumImages++;
		}
		if (tag.tag == 0x14 && tag.length == 2560 && tag.type == 7) {
			fRead.Seek(1248, SEEK_CUR);
			goto get2_256;
		}
		if (tag.tag == 0x1d) {
			int c;
			while ((c = fRead.Next<uint8>()) && c != EOF) {
				serial = serial * 10 + (isdigit(c) ? c - '0' : c % 10);
			}
		}
		if (tag.tag == 0x81 && tag.type == 4) {
			_Raw().data_offset = fRead.Next<uint32>();
			fRead.Seek(_Raw().data_offset + 41, SEEK_SET);
			_Raw().height = fRead.Next<uint16>() * 2;
			_Raw().width  = fRead.Next<uint16>();
			fFilters = 0x61616161;
		}
		if ((tag.tag == 0x81 && tag.type == 7)
			|| (tag.tag == 0x100 && tag.type == 7)
			|| (tag.tag == 0x280 && tag.type == 1)) {
			_Thumb().data_offset = fRead.Position();
			_Thumb().bytes = tag.length;
		}
		if (tag.tag == 0x88 && tag.type == 4
			&& (_Thumb().data_offset = fRead.Next<uint32>())) {
			_Thumb().data_offset += baseOffset;
		}
		if (tag.tag == 0x89 && tag.type == 4)
			_Thumb().bytes = fRead.Next<uint32>();
		if (tag.tag == 0x8c)
			fCurveOffset = fRead.Position() + 2112;
		if (tag.tag == 0x96)
			fCurveOffset = fRead.Position() + 2;
		if (tag.tag == 0x97) {
			for (uint32 i = 0; i < 4; i++) {
				ver97 = (ver97 << 4) + fRead.Next<uint8>() - '0';
			}
			switch (ver97) {
				case 0x100:
					fRead.Seek(68, SEEK_CUR);
					for (uint32 i = 0; i < 4; i++) {
						fMeta.camera_multipliers[(i >> 1) | ((i & 1) << 1)]
							= fRead.Next<uint16>();
					}
					break;
				case 0x102:
					fRead.Seek(6, SEEK_CUR);
					goto get2_rggb;
				case 0x103:
					fRead.Seek(16, SEEK_CUR);
					for (uint32 i = 0; i < 4; i++) {
						fMeta.camera_multipliers[i] = fRead.Next<uint16>();
					}
					break;
			}
			if (ver97 >> 8 == 2) {
				if (ver97 != 0x205)
					fRead.Seek(280, SEEK_CUR);
				fRead(buf97, sizeof(buf97));
			}
		}
		if (tag.tag == 0xa7 && ver97 >> 8 == 2) {
			ci = xlat[0][serial & 0xff];
			cj = xlat[1][fRead.Next<uint8>() ^ fRead.Next<uint8>()
				^ fRead.Next<uint8>() ^ fRead.Next<uint8>()];
			ck = 0x60;
			for (uint32 i = 0; i < 324; i++) {
				buf97[i] ^= (cj += ci * ck++);
			}
			for (uint32 i = 0; i < 4; i++) {
				uint16* data = (uint16*)(buf97
					+ (ver97 == 0x205 ? 14 : 6) + i * 2);

				if (fRead.IsSwapping()) {
					fMeta.camera_multipliers[i ^ (i >> 1)]
						= __swap_int16(*data);
				} else {
					fMeta.camera_multipliers[i ^ (i >> 1)] = *data;
				}
			}
		}
		if (tag.tag == 0x200 && tag.length == 4) {
			fMeta.black = (fRead.Next<uint16>() + fRead.Next<uint16>()
				+ fRead.Next<uint16>() + fRead.Next<uint16>()) / 4;
		}
		if (tag.tag == 0x201 && tag.length == 4)
			goto get2_rggb;
		if (tag.tag == 0x401 && tag.length == 4) {
			fMeta.black = (fRead.Next<uint32>() + fRead.Next<uint32>()
				+ fRead.Next<uint32>() + fRead.Next<uint32>()) / 4;
		}
		if (tag.tag == 0xe01) {
			// Nikon Capture Note
			bool previousSwap = fRead.IsSwapping();
			fRead.SetSwap(B_HOST_IS_BENDIAN != 0);
				// this chunk is always in little endian

			off_t offset = 22;
			fRead.Seek(offset, SEEK_CUR);

			int32 i = 0;

			for (; offset + 22 < tag.length; offset += 22 + i) {
				tag.tag = fRead.Next<uint32>();
				fRead.Seek(14, SEEK_CUR);
				i = fRead.Next<uint32>() - 4;
				if (tag.tag == 0x76a43207)
					fMeta.flip = fRead.Next<uint16>();
				else
					fRead.Seek(i, SEEK_CUR);
			}

			fRead.SetSwap(previousSwap);
		}
		if (tag.tag == 0xe80 && tag.length == 256 && tag.type == 7) {
			fRead.Seek(48, SEEK_CUR);
			fMeta.camera_multipliers[0]
				= fRead.Next<uint16>() * 508 * 1.078 / 0x10000;
			fMeta.camera_multipliers[2]
				= fRead.Next<uint16>() * 382 * 1.173 / 0x10000;
		}
		if (tag.tag == 0xf00 && tag.type == 7) {
			if (tag.length == 614)
				fRead.Seek(176, SEEK_CUR);
			else if (tag.length == 734 || tag.length == 1502)
				fRead.Seek(148, SEEK_CUR);
			else
				goto next;
			goto get2_256;
		}
		if (tag.tag == 0x1011 && tag.length == 9 && fUseCameraWhiteBalance) {
			for (uint32 i = 0; i < 3; i++) {
				for (uint32 j = 0; j < 3; j++) {
					fMeta.rgb_camera[i][j] = fRead.Next<int16>() / 256.0;
				}
			}
			fRawColor = fMeta.rgb_camera[0][0] < 1;
		}
		if (tag.tag == 0x1012 && tag.length == 4) {
			fMeta.black = 0;
			for (uint32 i = 0; i < 4; i++) {
				fMeta.black += fRead.Next<uint16>() << 2;
			}
		}
		if (tag.tag == 0x1017)
			fMeta.camera_multipliers[0] = fRead.Next<uint16>() / 256.0;
		if (tag.tag == 0x1018)
			fMeta.camera_multipliers[2] = fRead.Next<uint16>() / 256.0;

		if (tag.tag == 0x2011 && tag.length == 2) {
get2_256:
			bool previousSwap = fRead.IsSwapping();
			fRead.SetSwap(B_HOST_IS_LENDIAN != 0);
				// this chunk is always in big endian

			fMeta.camera_multipliers[0] = fRead.Next<uint16>() / 256.0;
			fMeta.camera_multipliers[2] = fRead.Next<uint16>() / 256.0;

			fRead.SetSwap(previousSwap);
		}

		if (tag.tag == 0x2020)
			_ParseThumbTag(baseOffset, 257, 258);
		if (tag.tag == 0xb028) {
			fRead.Seek(fRead.Next<uint32>(), SEEK_SET);
			_ParseThumbTag(baseOffset, 136, 137);
		}

		if (tag.tag == 0x4001) {
			{
				off_t offset = tag.length == 582 ? 50 : tag.length == 653
					? 68 : 126;
				fRead.Seek(offset, SEEK_CUR);
			}
get2_rggb:
			for (uint32 i = 0; i < 4; i++) {
				fMeta.camera_multipliers[i ^ (i >> 1)] = fRead.Next<uint16>();
			}
		}
	
next:
    	fRead.Seek(nextOffset, SEEK_SET);
	}

quit:
	fRead.SetSwap(originalSwap);
}


void
DCRaw::_ParseEXIF(off_t baseOffset)
{
	bool kodak = !strncmp(fMeta.manufacturer, "EASTMAN", 7);

	uint16 entries;
	fRead(entries);

	while (entries--) {
		off_t nextOffset;
		tiff_tag tag;
		_ParseTIFFTag(baseOffset, tag, nextOffset);
		TAG("EXIF tag %u (type %u, length %lu)\n", tag.tag, tag.type,
			tag.length);

		switch (tag.tag) {
#if 0
			default:
				printf("  unhandled EXIF tag %u\n", tag.tag);
				break;
#endif
			case 33434:
				fMeta.shutter = fRead.NextDouble(TIFF_FRACTION_TYPE);
				break;
			case 33437:
				fMeta.aperture = fRead.NextDouble(TIFF_FRACTION_TYPE);
				break;
			case 34855:
				fMeta.iso_speed = fRead.Next<uint16>();
				break;
			case 36867:
			case 36868:
				fMeta.timestamp = _ParseTIFFTimestamp(false);
				break;
			case 37377:
			{
				double expo;
				if ((expo = -fRead.NextDouble(TIFF_FRACTION_TYPE)) < 128)
					fMeta.shutter = pow(2, expo);
				break;
			}
			case 37378:
				fMeta.aperture
					= pow(2, fRead.NextDouble(TIFF_FRACTION_TYPE) / 2);
				break;
			case 37386:
				fMeta.focal_length = fRead.NextDouble(TIFF_FRACTION_TYPE);
				break;
			case 37500:
				_ParseManufacturerTag(baseOffset);
				break;
			case 40962:
				if (kodak)
					_Raw().width = fRead.Next<uint32>();
				break;
			case 40963:
				if (kodak)
					_Raw().height = fRead.Next<uint32>();
				break;
			case 41730:
				if (fRead.Next<uint32>() == 0x20002) {
					fEXIFFilters = 0;
					for (uint32 c = 0; c < 8; c += 2) {
						fEXIFFilters |= fRead.Next<uint8>() * 0x01010101 << c;
					}
				}
				break;
		}

		fRead.Seek(nextOffset, SEEK_SET);
	}
}


void
DCRaw::_ParseLinearTable(uint32 length)
{
	if (length > 0x1000)
		length = 0x1000;

	fRead.NextShorts(fCurve, length);

	for (uint32 i = length; i < 0x1000; i++) {
		fCurve[i] = fCurve[i - 1];
	}

	fMeta.maximum = fCurve[0xfff];
}


/*!	This (lengthy) method contains fixes for the values in the image data to
	be able to actually read the image data correctly.
*/
void
DCRaw::_FixupValues()
{
	// PENTAX and SAMSUNG section
	// (Samsung sells rebranded Pentax cameras)

	if (_IsPentax() || _IsSamsung()) {
		if (fInputWidth == 3936 && fInputHeight == 2624) {
			// Pentax K10D and Samsumg GX10
			fInputWidth = 3896;
			fInputHeight = 2616;
		}
	}

	// CANON

	if (_IsCanon()) {
		bool isCR2 = false;
		if (strstr(fMeta.model, "EOS D2000C")) {
			fFilters = 0x61616161;
			fMeta.black = fCurve[200];
		}

		switch (_Raw().width) {
			case 2144:
				fInputHeight = 1550;
				fInputWidth = 2088;
				fTopMargin = 8;
				fLeftMargin = 4;
				if (!strcmp(fMeta.model, "PowerShot G1")) {
					fColors = 4;
					fFilters = 0xb4b4b4b4;
				}
				break;

			case 2224:
				fInputHeight = 1448;
				fInputWidth = 2176;
				fTopMargin = 6;
				fLeftMargin = 48;
				break;

			case 2376:
				fInputHeight = 1720;
				fInputWidth = 2312;
				fTopMargin = 6;
				fLeftMargin = 12;
				break;

			case 2672:
				fInputHeight = 1960;
				fInputWidth = 2616;
				fTopMargin = 6;
				fLeftMargin = 12;
				break;

			case 3152:
				fInputHeight = 2056;
				fInputWidth = 3088;
				fTopMargin = 12;
				fLeftMargin = 64;
				if (fUniqueID == 0x80000170)
					_AdobeCoefficients("Canon", "EOS 300D");
				fMeta.maximum = 0xfa0;
				break;

			case 3160:
				fInputHeight = 2328;
				fInputWidth = 3112;
				fTopMargin = 12;
				fLeftMargin = 44;
				break;

			case 3344:
				fInputHeight = 2472;
				fInputWidth = 3288;
				fTopMargin = 6;
				fLeftMargin = 4;
				break;

			case 3516:
				fTopMargin = 14;
				fLeftMargin = 42;
				if (fUniqueID == 0x80000189)
					_AdobeCoefficients("Canon", "EOS 350D");
				isCR2 = true;
				break;

			case 3596:
				fTopMargin = 12;
				fLeftMargin = 74;
				isCR2 = true;
				break;

			case 3948:
				fTopMargin = 18;
				fLeftMargin = 42;
				fInputHeight -= 2;
				if (fUniqueID == 0x80000236)
					_AdobeCoefficients("Canon", "EOS 400D");
				isCR2 = true;
				break;

			case 3984:
				fTopMargin  = 20;
				fLeftMargin = 76;
				fInputHeight -= 2;
				fMeta.maximum = 0x3bb0;
				isCR2 = true;
				break;

			case 4476:
				fTopMargin  = 34;
				fLeftMargin = 90;
				fMeta.maximum = 0xe6c;
				isCR2 = true;
				break;

			case 5108:
				fTopMargin  = 13;
				fLeftMargin = 98;
				fMeta.maximum = 0xe80;
				isCR2 = true;
				break;
		}

		if (isCR2) {
			fInputHeight -= fTopMargin;
			fInputWidth -= fLeftMargin;
		}
	}

	// Olympus

	if (_IsOlympus()) {
		if (!strcmp(fMeta.model,"E-300") || !strcmp(fMeta.model,"E-500")) {
			fInputWidth -= 20;
			fMeta.maximum = 0xfc30;
			//if (load_raw == &CLASS unpacked_load_raw) black = 0;
		}
	}
}


//	#pragma mark - Image Conversion


void
DCRaw::_ScaleColors()
{
	if (fProgressMonitor != NULL)
		fProgressMonitor("Scale Colors", 5, fProgressData);

	int c, val, sum[8];
	uint32 row, col, x, y;
	double dsum[8], dmin, dmax;
	float scale_mul[4];

	if (fUseCameraWhiteBalance && fMeta.camera_multipliers[0] != -1) {
		memset(sum, 0, sizeof(sum));
		for (row = 0; row < 8; row++) {
			for (col = 0; col < 8; col++) {
				c = FC(row, col);
				if ((val = fWhite[row][col] - fMeta.black) > 0)
					sum[c] += val;
				sum[c + 4]++;
			}
		}

		if (sum[0] && sum[1] && sum[2] && sum[3]) {
			for (int c = 0; c < 4; c++) {
				fMeta.pre_multipliers[c] = (float)sum[c+4] / sum[c];
			}
		} else if (fMeta.camera_multipliers[0] && fMeta.camera_multipliers[2]) {
			memcpy(fMeta.pre_multipliers, fMeta.camera_multipliers,
				sizeof(fMeta.pre_multipliers));
		} else
			fprintf(stderr, "Cannot use camera white balance.\n");
	} else if (fUseAutoWhiteBalance) {
		memset(dsum, 0, sizeof(dsum));
		for (row = 0; row < fOutputHeight - 7; row += 8) {
			for (col = 0; col < fOutputWidth - 7; col += 8) {
				memset(sum, 0, sizeof(sum));
				for (y = row; y < row + 8; y++) {
					for (x = col; x < col + 8; x++) {
						for (int c = 0; c < 4; c++) {
							val = fImageData[y * fOutputWidth + x][c];
							if (!val)
								continue;
							if (val > fMeta.maximum - 25)
								goto skip_block;
							val -= fMeta.black;
							if (val < 0)
								val = 0;
							sum[c] += val;
							sum[c+4]++;
						}
					}
				}

				for (c=0; c < 8; c++) {
					dsum[c] += sum[c];
				}

			skip_block:
				continue;
			}
		}
		for (int c = 0; c < 4; c++) {
			if (dsum[c])
				fMeta.pre_multipliers[c] = dsum[c + 4] / dsum[c];
		}
	}


	if (fUserMultipliers[0]) {
		memcpy(fMeta.pre_multipliers, fUserMultipliers,
			sizeof(fMeta.pre_multipliers));
	}
	if (fMeta.pre_multipliers[3] == 0)
		fMeta.pre_multipliers[3] = fColors < 4 ? fMeta.pre_multipliers[1] : 1;

#if 0
	int dblack = fMeta.black;
#endif
	if (fThreshold)
		_WaveletDenoise();

	fMeta.maximum -= fMeta.black;
	for (dmin = DBL_MAX, dmax = c = 0; c < 4; c++) {
		if (dmin > fMeta.pre_multipliers[c])
			dmin = fMeta.pre_multipliers[c];
		if (dmax < fMeta.pre_multipliers[c])
			dmax = fMeta.pre_multipliers[c];
	}

	if (!fHighlight)
		dmax = dmin;

	for (int c = 0; c < 4; c++) {
		scale_mul[c] = (fMeta.pre_multipliers[c] /= dmax) * 65535.0
			/ fMeta.maximum;
	}

#if 0
	if (1/*verbose*/) {
		fprintf(stderr, "Scaling with black %d, multipliers", dblack);
		for (int c = 0; c < 4; c++) {
			fprintf(stderr, " %f", fMeta.pre_multipliers[c]);
		}
		fputc('\n', stderr);
	}
#endif

	for (row = 0; row < fOutputHeight; row++) {
		for (col = 0; col < fOutputWidth; col++) {
			for (int c = 0; c < 4; c++) {
				val = fImageData[row * fOutputWidth + col][c];
				if (!val)
					continue;
				val -= fMeta.black;
				val = int(val * scale_mul[c]);
				fImageData[row * fOutputWidth + col][c] = CLIP(val);
			}
		}
	}
}


void
DCRaw::_WaveletDenoise()
{
	if (fProgressMonitor != NULL)
		fProgressMonitor("Wavelet Denoise", 8, fProgressData);

	float *fimg, *temp, mul[2], avg, diff;
	int32 scale = 1, sh, c, i, j, k, m, row, col, size, numColors, dim = 0;
	int32 wlast;
	ushort *window[4];
	// Daubechies 9-tap/7-tap filter
	static const float wlet[] =	{ 1.149604398, -1.586134342,
		-0.05298011854, 0.8829110762, 0.4435068522 };

	while ((fMeta.maximum << scale) < 0x10000) {
		scale++;
	}
	fMeta.maximum <<= fMeta.maximum << --scale;
	fMeta.black <<= scale;

	while ((1UL << dim) < fOutputWidth || (1UL << dim) < fOutputHeight) {
		dim++;
	}

	fimg = (float *)calloc((1UL << dim*2) + (1UL << dim) + 2, sizeof *fimg);
	if (fimg == NULL)
		return;

	temp = fimg + (1 << dim * 2) + 1;
	numColors = fColors;
	if (numColors == 3 && fFilters)
		numColors++;

	for (c = 0; c < numColors; c++) {
		// denoise R,G1,B,G3 individually
		for (row = 0; row < (int32)fOutputHeight; row++) {
			for (col = 0; col < (int32)fOutputWidth; col++) {
				fimg[(row << dim) + col]
					= fImageData[row * fOutputWidth + col][c] << scale;
			}
		}
		for (size = 1UL << dim; size > 1; size >>= 1) {
			for (sh = 0; sh <= dim; sh += dim) {
				for (i = 0; i < size; i++) {
					for (j = 0; j < size; j++) {
						temp[j] = fimg[(i << (dim - sh)) + (j << sh)];
					}
					for (k = 1; k < 5; k += 2) {
						temp[size] = temp[size - 2];
						for (m = 1; m < size; m += 2) {
							temp[m] += wlet[k] * (temp[m - 1] + temp[m + 1]);
						}
						temp[-1] = temp[1];
						for (m = 0; m < size; m += 2) {
							temp[m] += wlet[k + 1]
								* (temp[m - 1] + temp[m + 1]);
						}
					}
					for (m = 0; m < size; m++) {
						temp[m] *= (m & 1) ? 1 / wlet[0] : wlet[0];
					}
					for (j = k = 0; j < size; j++, k += 2) {
						if (k == size)
							k = 1;
						fimg[(i << (dim - sh)) + (j << sh)] = temp[k];
					}
				}
			}
		}

		for (i = 0; i < (1 << dim * 2); i++) {
			if (fimg[i] < -fThreshold)
				fimg[i] += fThreshold;
			else if (fimg[i] > fThreshold)
				fimg[i] -= fThreshold;
			else
				fimg[i] = 0;
		}

		for (size = 2; size <= (1 << dim); size <<= 1) {
			for (sh = dim; sh >= 0; sh -= dim) {
				for (i = 0; i < size; i++) {
					for (j = k = 0; j < size; j++, k += 2) {
						if (k == size)
							k = 1;
						temp[k] = fimg[(i << (dim - sh)) + (j << sh)];
					}
					for (m = 0; m < size; m++) {
						temp[m] *= (m & 1) ? wlet[0] : 1 / wlet[0];
					}
					for (k = 3; k > 0; k -= 2) {
						temp[-1] = temp[1];
						for (m = 0; m < size; m += 2) {
							temp[m] -= wlet[k + 1]
								* (temp[m - 1] + temp[m + 1]);
						}
						temp[size] = temp[size - 2];
						for (m = 1; m < size; m += 2) {
							temp[m] -= wlet[k] * (temp[m - 1] + temp[m + 1]);
						}
					}
					for (j = 0; j < size; j++) {
						fimg[(i << (dim - sh)) + (j << sh)] = temp[j];
					}
				}
			}
		}

		for (row = 0; row < (int32)fOutputHeight; row++) {
			for (col = 0; col < (int32)fOutputWidth; col++) {
				fImageData[row * fOutputWidth + col][c]
					= (uint16)CLIP(fimg[(row << dim) + col] + 0.5);
			}
		}
	}

	if (fFilters && fColors == 3) {
		// pull G1 and G3 closer together
		for (row = 0; row < 2; row++) {
			mul[row] = 0.125 * fMeta.pre_multipliers[FC(row + 1, 0) | 1]
				/ fMeta.pre_multipliers[FC(row, 0) | 1];
		}
		for (i = 0; i < 4; i++) {
			window[i] = (ushort *)fimg + fInputWidth * i;
		}
		for (wlast = -1, row = 1; row < (int32)fInputHeight - 1; row++) {
			while (wlast < (int32)row + 1) {
				for (wlast++, i = 0; i < 4; i++) {
					window[(i + 3) & 3] = window[i];
				}
				for (col = FC(wlast, 1) & 1; col < (int32)fInputWidth;
						col += 2) {
					window[2][col] = _Bayer(col, wlast);
				}
			}

			for (col = (FC(row, 0) & 1) + 1; col < (int32)fInputWidth - 1;
					col += 2) {
				avg = ( window[0][col - 1] + window[0][col + 1]
					+ window[2][col - 1] + window[2][col + 1] - fMeta.black * 4)
					* mul[row & 1] + (window[1][col] - fMeta.black) * 0.5
					+ fMeta.black;
				diff = _Bayer(col, row) - avg;

				if (diff < -fThreshold / M_SQRT2)
					diff += fThreshold / M_SQRT2;
				else if (diff > fThreshold / M_SQRT2)
					diff -= fThreshold / M_SQRT2;
				else
					diff = 0;
				_Bayer(col, row) = (uint16)CLIP(avg + diff + 0.5);
			}
		}
	}

	free(fimg);
}


void
DCRaw::_PreInterpolate()
{
	if (fProgressMonitor != NULL)
		fProgressMonitor("Pre-Interpolate", 10, fProgressData);

	uint32 row, col;

	if (fShrink) {
		if (fHalfSize) {
			fInputHeight = fOutputHeight;
			fInputWidth = fOutputWidth;
			fFilters = 0;
		} else {
			uint16 (*data)[4] = (uint16 (*)[4])calloc(fInputHeight
				* fInputWidth, sizeof(*data));
			if (data == NULL)
				throw (status_t)B_NO_MEMORY;

			for (row = 0; row < fInputHeight; row++) {
				for (col = 0; col < fInputWidth; col++) {
					data[row * fInputWidth + col][FC(row, col)]
						= _Bayer(col, row);
				}
			}

			free(fImageData);
			fImageData = data;
			fShrink = 0;
		}
	}

	if (fFilters && fColors == 3) {
//		if ((mix_green = four_color_rgb))
//			fColors++;
//		else
		{
			for (row = FC(1, 0) >> 1; row < fInputHeight; row += 2) {
				for (col = FC(row, 1) & 1; col < fInputWidth; col += 2) {
					fImageData[row * fInputWidth + col][1]
						= fImageData[row * fInputWidth + col][3];
				}
			}
			fFilters &= ~((fFilters & 0x55555555) << 1);
		}
	}
}


void
DCRaw::_CameraToCIELab(ushort cam[4], float lab[3])
{
	if (cam == NULL) {
		for (uint32 i = 0; i < 0x10000; i++) {
			float r = i / 65535.0;
			cbrt[i] = r > 0.008856 ? pow(r, 1 / 3.0) : 7.787 * r + 16 / 116.0;
		}
		for (uint32 i = 0; i < 3; i++) {
			for (uint32 j = 0; j < fColors; j++) {
				xyz_cam[i][j] = 0;
				for (uint32 k = 0; k < 3; k++) {
					xyz_cam[i][j] += xyz_rgb[i][k] * fMeta.rgb_camera[k][j]
						/ kD65White[i];
				}
			}
		}
	} else {
		float xyz[3];
		xyz[0] = xyz[1] = xyz[2] = 0.5;
		for (uint32 c = 0; c < fColors; c++) {
			xyz[0] += xyz_cam[0][c] * cam[c];
			xyz[1] += xyz_cam[1][c] * cam[c];
			xyz[2] += xyz_cam[2][c] * cam[c];
		}
		xyz[0] = cbrt[CLIP((int) xyz[0])];
		xyz[1] = cbrt[CLIP((int) xyz[1])];
		xyz[2] = cbrt[CLIP((int) xyz[2])];
		lab[0] = 116 * xyz[1] - 16;
		lab[1] = 500 * (xyz[0] - xyz[1]);
		lab[2] = 200 * (xyz[1] - xyz[2]);
	}
}


void
DCRaw::_CameraXYZCoefficients(double cameraXYZ[4][3])
{
	double cam_rgb[4][3], inverse[4][3], num;
	uint32 i, j, k;

	// Multiply out XYZ colorspace
	for (i = 0; i < fColors; i++)	{
		for (j = 0; j < 3; j++) {
			for (cam_rgb[i][j] = k = 0; k < 3; k++) {
				cam_rgb[i][j] += cameraXYZ[i][k] * xyz_rgb[k][j];
			}
		}
	}

	// Normalize cam_rgb so that cam_rgb * (1,1,1) is (1,1,1,1)
	for (i = 0; i < fColors; i++) {
		for (num = j = 0; j < 3; j++) {
			num += cam_rgb[i][j];
		}
		for (j = 0; j < 3; j++) {
			cam_rgb[i][j] /= num;
		}
		fMeta.pre_multipliers[i] = 1 / num;
	}

	_PseudoInverse(cam_rgb, inverse, fColors);

	fRawColor = false;
	for (i = 0; i < 3; i++) {
		for (j=0; j < fColors; j++) {
			fMeta.rgb_camera[i][j] = inverse[j][i];
		}
	}
}


/*!	Thanks to Adobe for providing these excellent CAM -> XYZ matrices!
*/
void
DCRaw::_AdobeCoefficients(const char *make, const char *model)
{
	static const struct {
		const char *prefix;
		short black, trans[12];
	} table[] = {
		{ "Canon EOS D2000", 0,
			{ 24542,-10860,-3401,-1490,11370,-297,2858,-605,3225 }},
		{ "Canon EOS D6000", 0,
			{ 20482,-7172,-3125,-1033,10410,-285,2542,226,3136 }},
		{ "Canon EOS D30", 0,
			{ 9805,-2689,-1312,-5803,13064,3068,-2438,3075,8775 }},
		{ "Canon EOS D60", 0,
			{ 6188,-1341,-890,-7168,14489,2937,-2640,3228,8483 }},
		{ "Canon EOS 5D", 0,
			{ 6347,-479,-972,-8297,15954,2480,-1968,2131,7649 }},
		{ "Canon EOS 20Da", 0,
			{ 14155,-5065,-1382,-6550,14633,2039,-1623,1824,6561 }},
		{ "Canon EOS 20D", 0,
			{ 6599,-537,-891,-8071,15783,2424,-1983,2234,7462 }},
		{ "Canon EOS 30D", 0,
			{ 6257,-303,-1000,-7880,15621,2396,-1714,1904,7046 }},
		{ "Canon EOS 350D", 0,
			{ 6018,-617,-965,-8645,15881,2975,-1530,1719,7642 }},
		{ "Canon EOS 400D", 0,
			{ 7054,-1501,-990,-8156,15544,2812,-1278,1414,7796 }},
		{ "Canon EOS-1Ds Mark II", 0,
			{ 6517,-602,-867,-8180,15926,2378,-1618,1771,7633 }},
		{ "Canon EOS-1D Mark II N", 0,
			{ 6240,-466,-822,-8180,15825,2500,-1801,1938,8042 }},
		{ "Canon EOS-1D Mark II", 0,
			{ 6264,-582,-724,-8312,15948,2504,-1744,1919,8664 }},
		{ "Canon EOS-1DS", 0,
			{ 4374,3631,-1743,-7520,15212,2472,-2892,3632,8161 }},
		{ "Canon EOS-1D", 0,
			{ 6806,-179,-1020,-8097,16415,1687,-3267,4236,7690 }},
		{ "Canon EOS", 0,
			{ 8197,-2000,-1118,-6714,14335,2592,-2536,3178,8266 }},
		{ "Canon PowerShot A50", 0,
			{ -5300,9846,1776,3436,684,3939,-5540,9879,6200,-1404,11175,217 }},
		{ "Canon PowerShot A5", 0,
			{ -4801,9475,1952,2926,1611,4094,-5259,10164,5947,-1554,10883,547 }},
		{ "Canon PowerShot G1", 0,
			{ -4778,9467,2172,4743,-1141,4344,-5146,9908,6077,-1566,11051,557 }},
		{ "Canon PowerShot G2", 0,
			{ 9087,-2693,-1049,-6715,14382,2537,-2291,2819,7790 }},
		{ "Canon PowerShot G3", 0,
			{ 9212,-2781,-1073,-6573,14189,2605,-2300,2844,7664 }},
		{ "Canon PowerShot G5", 0,
			{ 9757,-2872,-933,-5972,13861,2301,-1622,2328,7212 }},
		{ "Canon PowerShot G6", 0,
			{ 9877,-3775,-871,-7613,14807,3072,-1448,1305,7485 }},
		{ "Canon PowerShot Pro1", 0,
			{ 10062,-3522,-999,-7643,15117,2730,-765,817,7323 }},
		{ "Canon PowerShot Pro70", 34,
			{ -4155,9818,1529,3939,-25,4522,-5521,9870,6610,-2238,10873,1342 }},
		{ "Canon PowerShot Pro90", 0,
			{ -4963,9896,2235,4642,-987,4294,-5162,10011,5859,-1770,11230,577 }},
		{ "Canon PowerShot S30", 0,
			{ 10566,-3652,-1129,-6552,14662,2006,-2197,2581,7670 }},
		{ "Canon PowerShot S40", 0,
			{ 8510,-2487,-940,-6869,14231,2900,-2318,2829,9013 }},
		{ "Canon PowerShot S45", 0,
			{ 8163,-2333,-955,-6682,14174,2751,-2077,2597,8041 }},
		{ "Canon PowerShot S50", 0,
			{ 8882,-2571,-863,-6348,14234,2288,-1516,2172,6569 }},
		{ "Canon PowerShot S60", 0,
			{ 8795,-2482,-797,-7804,15403,2573,-1422,1996,7082 }},
		{ "Canon PowerShot S70", 0,
			{ 9976,-3810,-832,-7115,14463,2906,-901,989,7889 }},
		{ "Canon PowerShot A610", 0, /* DJC */
			{ 15591,-6402,-1592,-5365,13198,2168,-1300,1824,5075 }},
		{ "Canon PowerShot A620", 0, /* DJC */
			{ 15265,-6193,-1558,-4125,12116,2010,-888,1639,5220 }},
		{ "Canon PowerShot S3 IS", 0, /* DJC */
			{ 14062,-5199,-1446,-4712,12470,2243,-1286,2028,4836 }},
		{ "Contax N Digital", 0,
			{ 7777,1285,-1053,-9280,16543,2916,-3677,5679,7060 }},
		{ "EPSON R-D1", 0,
			{ 6827,-1878,-732,-8429,16012,2564,-704,592,7145 }},
		{ "FUJIFILM FinePix E550", 0,
			{ 11044,-3888,-1120,-7248,15168,2208,-1531,2277,8069 }},
		{ "FUJIFILM FinePix E900", 0,
			{ 9183,-2526,-1078,-7461,15071,2574,-2022,2440,8639 }},
		{ "FUJIFILM FinePix F8", 0,
			{ 11044,-3888,-1120,-7248,15168,2208,-1531,2277,8069 }},
		{ "FUJIFILM FinePix F7", 0,
			{ 10004,-3219,-1201,-7036,15047,2107,-1863,2565,7736 }},
		{ "FUJIFILM FinePix S20Pro", 0,
			{ 10004,-3219,-1201,-7036,15047,2107,-1863,2565,7736 }},
		{ "FUJIFILM FinePix S2Pro", 128,
			{ 12492,-4690,-1402,-7033,15423,1647,-1507,2111,7697 }},
		{ "FUJIFILM FinePix S3Pro", 0,
			{ 11807,-4612,-1294,-8927,16968,1988,-2120,2741,8006 }},
		{ "FUJIFILM FinePix S5000", 0,
			{ 8754,-2732,-1019,-7204,15069,2276,-1702,2334,6982 }},
		{ "FUJIFILM FinePix S5100", 0,
			{ 11940,-4431,-1255,-6766,14428,2542,-993,1165,7421 }},
		{ "FUJIFILM FinePix S5500", 0,
			{ 11940,-4431,-1255,-6766,14428,2542,-993,1165,7421 }},
		{ "FUJIFILM FinePix S5200", 0,
			{ 9636,-2804,-988,-7442,15040,2589,-1803,2311,8621 }},
		{ "FUJIFILM FinePix S5600", 0,
			{ 9636,-2804,-988,-7442,15040,2589,-1803,2311,8621 }},
		{ "FUJIFILM FinePix S6", 0,
			{ 12628,-4887,-1401,-6861,14996,1962,-2198,2782,7091 }},
		{ "FUJIFILM FinePix S7000", 0,
			{ 10190,-3506,-1312,-7153,15051,2238,-2003,2399,7505 }},
		{ "FUJIFILM FinePix S9000", 0,
			{ 10491,-3423,-1145,-7385,15027,2538,-1809,2275,8692 }},
		{ "FUJIFILM FinePix S9500", 0,
			{ 10491,-3423,-1145,-7385,15027,2538,-1809,2275,8692 }},
		{ "FUJIFILM FinePix S9100", 0,
			{ 12343,-4515,-1285,-7165,14899,2435,-1895,2496,8800 }},
		{ "FUJIFILM FinePix S9600", 0,
			{ 12343,-4515,-1285,-7165,14899,2435,-1895,2496,8800 }},
		{ "Imacon Ixpress", 0,	/* DJC */
			{ 7025,-1415,-704,-5188,13765,1424,-1248,2742,6038 }},
		{ "KODAK NC2000", 0,	/* DJC */
			{ 16475,-6903,-1218,-851,10375,477,2505,-7,1020 }},
		{ "Kodak DCS315C", 8,
			{ 17523,-4827,-2510,756,8546,-137,6113,1649,2250 }},
		{ "Kodak DCS330C", 8,
			{ 20620,-7572,-2801,-103,10073,-396,3551,-233,2220 }},
		{ "KODAK DCS420", 0,
			{ 10868,-1852,-644,-1537,11083,484,2343,628,2216 }},
		{ "KODAK DCS460", 0,
			{ 10592,-2206,-967,-1944,11685,230,2206,670,1273 }},
		{ "KODAK EOSDCS1", 0,
			{ 10592,-2206,-967,-1944,11685,230,2206,670,1273 }},
		{ "KODAK EOSDCS3B", 0,
			{ 9898,-2700,-940,-2478,12219,206,1985,634,1031 }},
		{ "Kodak DCS520C", 180,
			{ 24542,-10860,-3401,-1490,11370,-297,2858,-605,3225 }},
		{ "Kodak DCS560C", 188,
			{ 20482,-7172,-3125,-1033,10410,-285,2542,226,3136 }},
		{ "Kodak DCS620C", 180,
			{ 23617,-10175,-3149,-2054,11749,-272,2586,-489,3453 }},
		{ "Kodak DCS620X", 185,
			{ 13095,-6231,154,12221,-21,-2137,895,4602,2258 }},
		{ "Kodak DCS660C", 214,
			{ 18244,-6351,-2739,-791,11193,-521,3711,-129,2802 }},
		{ "Kodak DCS720X", 0,
			{ 11775,-5884,950,9556,1846,-1286,-1019,6221,2728 }},
		{ "Kodak DCS760C", 0,
			{ 16623,-6309,-1411,-4344,13923,323,2285,274,2926 }},
		{ "Kodak DCS Pro SLR", 0,
			{ 5494,2393,-232,-6427,13850,2846,-1876,3997,5445 }},
		{ "Kodak DCS Pro 14nx", 0,
			{ 5494,2393,-232,-6427,13850,2846,-1876,3997,5445 }},
		{ "Kodak DCS Pro 14", 0,
			{ 7791,3128,-776,-8588,16458,2039,-2455,4006,6198 }},
		{ "Kodak ProBack645", 0,
			{ 16414,-6060,-1470,-3555,13037,473,2545,122,4948 }},
		{ "Kodak ProBack", 0,
			{ 21179,-8316,-2918,-915,11019,-165,3477,-180,4210 }},
		{ "KODAK P712", 0,
			{ 9658,-3314,-823,-5163,12695,2768,-1342,1843,6044 }},
		{ "KODAK P850", 0,
			{ 10511,-3836,-1102,-6946,14587,2558,-1481,1792,6246 }},
		{ "KODAK P880", 0,
			{ 12805,-4662,-1376,-7480,15267,2360,-1626,2194,7904 }},
		{ "Leaf CMost", 0,
			{ 3952,2189,449,-6701,14585,2275,-4536,7349,6536 }},
		{ "Leaf Valeo 6", 0,
			{ 3952,2189,449,-6701,14585,2275,-4536,7349,6536 }},
		{ "Leaf Aptus 65", 0,
			{ 7914,1414,-1190,-8777,16582,2280,-2811,4605,5562 }},
		{ "Leaf Aptus 75", 0,
			{ 7914,1414,-1190,-8777,16582,2280,-2811,4605,5562 }},
		{ "Leaf", 0,
			{ 8236,1746,-1314,-8251,15953,2428,-3673,5786,5771 }},
		{ "Micron 2010", 110,	/* DJC */
			{ 16695,-3761,-2151,155,9682,163,3433,951,4904 }},
		{ "Minolta DiMAGE 5", 0,
			{ 8983,-2942,-963,-6556,14476,2237,-2426,2887,8014 }},
		{ "Minolta DiMAGE 7Hi", 0,
			{ 11368,-3894,-1242,-6521,14358,2339,-2475,3056,7285 }},
		{ "Minolta DiMAGE 7", 0,
			{ 9144,-2777,-998,-6676,14556,2281,-2470,3019,7744 }},
		{ "Minolta DiMAGE A1", 0,
			{ 9274,-2547,-1167,-8220,16323,1943,-2273,2720,8340 }},
		{ "MINOLTA DiMAGE A200", 0,
			{ 8560,-2487,-986,-8112,15535,2771,-1209,1324,7743 }},
		{ "Minolta DiMAGE A2", 0,
			{ 9097,-2726,-1053,-8073,15506,2762,-966,981,7763 }},
		{ "Minolta DiMAGE Z2", 0,	/* DJC */
			{ 11280,-3564,-1370,-4655,12374,2282,-1423,2168,5396 }},
		{ "MINOLTA DYNAX 5", 0,
			{ 10284,-3283,-1086,-7957,15762,2316,-829,882,6644 }},
		{ "MINOLTA DYNAX 7", 0,
			{ 10239,-3104,-1099,-8037,15727,2451,-927,925,6871 }},
		{ "NIKON D100", 0,
			{ 5902,-933,-782,-8983,16719,2354,-1402,1455,6464 }},
		{ "NIKON D1H", 0,
			{ 7577,-2166,-926,-7454,15592,1934,-2377,2808,8606 }},
		{ "NIKON D1X", 0,
			{ 7702,-2245,-975,-9114,17242,1875,-2679,3055,8521 }},
		{ "NIKON D1", 0,	/* multiplied by 2.218750, 1.0, 1.148438 */
			{ 16772,-4726,-2141,-7611,15713,1972,-2846,3494,9521 }},
		{ "NIKON D2H", 0,
			{ 5710,-901,-615,-8594,16617,2024,-2975,4120,6830 }},
		{ "NIKON D2X", 0,
			{ 10231,-2769,-1255,-8301,15900,2552,-797,680,7148 }},
		{ "NIKON D40", 0,
			{ 6992,-1668,-806,-8138,15748,2543,-874,850,7897 }},
		{ "NIKON D50", 0,
			{ 7732,-2422,-789,-8238,15884,2498,-859,783,7330 }},
		{ "NIKON D70", 0,
			{ 7732,-2422,-789,-8238,15884,2498,-859,783,7330 }},
		{ "NIKON D80", 0,
			{ 8629,-2410,-883,-9055,16940,2171,-1490,1363,8520 }},
		{ "NIKON D200", 0,
			{ 8367,-2248,-763,-8758,16447,2422,-1527,1550,8053 }},
		{ "NIKON E950", 0,		/* DJC */
			{ -3746,10611,1665,9621,-1734,2114,-2389,7082,3064,3406,6116,-244 }},
		{ "NIKON E995", 0,	/* copied from E5000 */
			{ -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 }},
		{ "NIKON E2500", 0,
			{ -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 }},
		{ "NIKON E4300", 0, /* copied from Minolta DiMAGE Z2 */
			{ 11280,-3564,-1370,-4655,12374,2282,-1423,2168,5396 }},
		{ "NIKON E4500", 0,
			{ -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 }},
		{ "NIKON E5000", 0,
			{ -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 }},
		{ "NIKON E5400", 0,
			{ 9349,-2987,-1001,-7919,15766,2266,-2098,2680,6839 }},
		{ "NIKON E5700", 0,
			{ -5368,11478,2368,5537,-113,3148,-4969,10021,5782,778,9028,211 }},
		{ "NIKON E8400", 0,
			{ 7842,-2320,-992,-8154,15718,2599,-1098,1342,7560 }},
		{ "NIKON E8700", 0,
			{ 8489,-2583,-1036,-8051,15583,2643,-1307,1407,7354 }},
		{ "NIKON E8800", 0,
			{ 7971,-2314,-913,-8451,15762,2894,-1442,1520,7610 }},
		{ "OLYMPUS C5050", 0,
			{ 10508,-3124,-1273,-6079,14294,1901,-1653,2306,6237 }},
		{ "OLYMPUS C5060", 0,
			{ 10445,-3362,-1307,-7662,15690,2058,-1135,1176,7602 }},
		{ "OLYMPUS C7070", 0,
			{ 10252,-3531,-1095,-7114,14850,2436,-1451,1723,6365 }},
		{ "OLYMPUS C70", 0,
			{ 10793,-3791,-1146,-7498,15177,2488,-1390,1577,7321 }},
		{ "OLYMPUS C80", 0,
			{ 8606,-2509,-1014,-8238,15714,2703,-942,979,7760 }},
		{ "OLYMPUS E-10", 0,
			{ 12745,-4500,-1416,-6062,14542,1580,-1934,2256,6603 }},
		{ "OLYMPUS E-1", 0,
			{ 11846,-4767,-945,-7027,15878,1089,-2699,4122,8311 }},
		{ "OLYMPUS E-20", 0,
			{ 13173,-4732,-1499,-5807,14036,1895,-2045,2452,7142 }},
		{ "OLYMPUS E-300", 0,
			{ 7828,-1761,-348,-5788,14071,1830,-2853,4518,6557 }},
		{ "OLYMPUS E-330", 0,
			{ 8961,-2473,-1084,-7979,15990,2067,-2319,3035,8249 }},
		{ "OLYMPUS E-400", 0,
			{ 6169,-1483,-21,-7107,14761,2536,-2904,3580,8568 }},
		{ "OLYMPUS E-500", 0,
			{ 8136,-1968,-299,-5481,13742,1871,-2556,4205,6630 }},
		{ "OLYMPUS SP350", 0,
			{ 12078,-4836,-1069,-6671,14306,2578,-786,939,7418 }},
		{ "OLYMPUS SP3", 0,
			{ 11766,-4445,-1067,-6901,14421,2707,-1029,1217,7572 }},
		{ "OLYMPUS SP500UZ", 0,
			{ 9493,-3415,-666,-5211,12334,3260,-1548,2262,6482 }},
		{ "OLYMPUS SP510UZ", 0,
			{ 10593,-3607,-1010,-5881,13127,3084,-1200,1805,6721 }},
		{ "PENTAX *ist DL2", 0,
			{ 10504,-2438,-1189,-8603,16207,2531,-1022,863,12242 }},
		{ "PENTAX *ist DL", 0,
			{ 10829,-2838,-1115,-8339,15817,2696,-837,680,11939 }},
		{ "PENTAX *ist DS2", 0,
			{ 10504,-2438,-1189,-8603,16207,2531,-1022,863,12242 }},
		{ "PENTAX *ist DS", 0,
			{ 10371,-2333,-1206,-8688,16231,2602,-1230,1116,11282 }},
		{ "PENTAX *ist D", 0,
			{ 9651,-2059,-1189,-8881,16512,2487,-1460,1345,10687 }},
		{ "PENTAX K10D", 0,
			{ 9566,-2863,-803,-7170,15172,2112,-818,803,9705 }},
		{ "PENTAX K1", 0,
			{ 11095,-3157,-1324,-8377,15834,2720,-1108,947,11688 }},
		{ "Panasonic DMC-FZ30", 0,
			{ 10976,-4029,-1141,-7918,15491,2600,-1670,2071,8246 }},
		{ "Panasonic DMC-FZ50", 0,	/* aka "LEICA V-LUX1" */
			{ 7906,-2709,-594,-6231,13351,3220,-1922,2631,6537 }},
		{ "Panasonic DMC-L1", 0,	/* aka "LEICA DIGILUX 3" */
			{ 8054,-1885,-1025,-8349,16367,2040,-2805,3542,7629 }},
		{ "Panasonic DMC-LC1", 0,	/* aka "LEICA DIGILUX 2" */
			{ 11340,-4069,-1275,-7555,15266,2448,-2960,3426,7685 }},
		{ "Panasonic DMC-LX1", 0,	/* aka "LEICA D-LUX2" */
			{ 10704,-4187,-1230,-8314,15952,2501,-920,945,8927 }},
		{ "Panasonic DMC-LX2", 0,	/* aka "LEICA D-LUX3" */
			{ 8048,-2810,-623,-6450,13519,3272,-1700,2146,7049 }},
		{ "SAMSUNG GX-1", 0,
			{ 10504,-2438,-1189,-8603,16207,2531,-1022,863,12242 }},
		{ "Sinar", 0,		/* DJC */
			{ 16442,-2956,-2422,-2877,12128,750,-1136,6066,4559 }},
		{ "SONY DSC-F828", 491,
			{ 7924,-1910,-777,-8226,15459,2998,-1517,2199,6818,-7242,11401,3481 }},
		{ "SONY DSC-R1", 512,
			{ 8512,-2641,-694,-8042,15670,2526,-1821,2117,7414 }},
		{ "SONY DSC-V3", 0,
			{ 7511,-2571,-692,-7894,15088,3060,-948,1111,8128 }},
		{ "SONY DSLR-A100", 0,
			{ 9437,-2811,-774,-8405,16215,2290,-710,596,7181 }}
	};
	double cameraXYZ[4][3];

	for (uint32 i = 0; i < sizeof table / sizeof *table; i++) {
		if (!strncasecmp(model, table[i].prefix, strlen(table[i].prefix))) {
			if (table[i].black)
				fMeta.black = table[i].black;
			for (uint32 j = 0; j < 12; j++) {
				cameraXYZ[0][j] = table[i].trans[j] / 10000.0;
			}
			_CameraXYZCoefficients(cameraXYZ);
			break;
		}
	}
}


void
DCRaw::_BorderInterpolate(uint32 border)
{
	uint32 row, col, y, x, f, c, sum[8];

	for (row = 0; row < fInputHeight; row++) {
		for (col = 0; col < fInputWidth; col++) {
			if (col == border && row >= border && row < fInputHeight - border)
				col = fInputWidth - border;

			memset(sum, 0, sizeof(sum));

			for (y = row - 1; y != row + 2; y++) {
				for (x = col - 1; x != col + 2; x++) {
					if (y < fInputHeight && x < fInputWidth) {
						f = _FilterCoefficient(x, y);
						sum[f] += fImageData[y * fInputWidth + x][f];
						sum[f + 4]++;
					}
				}
			}

			f = _FilterCoefficient(col, row);

			for (c = 0; c < fColors; c++) {
				if (c != f && sum[c + 4]) {
					fImageData[row * fInputWidth + col][c]
						= sum[c] / sum[c + 4];
				}
			}
		}
	}
}


/*!	Adaptive Homogeneity-Directed interpolation is based on
	the work of Keigo Hirakawa, Thomas Parks, and Paul Lee.
*/
void
DCRaw::_AHDInterpolate()
{
	if (fProgressMonitor != NULL)
		fProgressMonitor("Interpolate", 20, fProgressData);

#define TS 256		/* Tile Size */

	int i, j, tr, tc, fc, c, d, val, hm[2];
	uint32 top, left, row, col;
	ushort (*pix)[4], (*rix)[3];
	static const int dir[4] = { -1, 1, -TS, TS };
	unsigned ldiff[2][4], abdiff[2][4], leps, abeps;
	float flab[3];
	ushort (*rgb)[TS][TS][3];
	short (*lab)[TS][TS][3];
	char (*homo)[TS][TS], *buffer;

	_BorderInterpolate(3);
	buffer = (char *)malloc(26 * TS * TS);		/* 1664 kB */
	if (buffer == NULL)
		throw (status_t)B_NO_MEMORY;

	rgb = (ushort(*)[TS][TS][3])buffer;
	lab = (short (*)[TS][TS][3])(buffer + 12 * TS * TS);
	homo = (char (*)[TS][TS])(buffer + 24 * TS * TS);
	float percentage = 20;
	float percentageStep = 70.0f / (fInputHeight / (TS - 6));

	for (top = 0; top < fInputHeight; top += TS - 6) {
		if (fProgressMonitor) {
			fProgressMonitor("Interpolate", percentage, fProgressData);
			percentage += percentageStep;
		}

		for (left = 0; left < fInputWidth; left += TS - 6) {
			memset(rgb, 0, 12 * TS * TS);

			/* Interpolate green horizontally and vertically: */
			for (row = top < 2 ? 2 : top; row < top + TS
					&& row < fInputHeight - 2; row++) {
				col = left + (FC(row, left) == 1);
				if (col < 2)
					col += 2;
				for (fc = FC(row, col); col < left + TS
						&& col < fInputWidth - 2; col += 2) {
					pix = fImageData + row * fInputWidth + col;
					val = ((pix[-1][1] + pix[0][fc] + pix[1][1]) * 2
						- pix[-2][fc] - pix[2][fc]) >> 2;
					rgb[0][row - top][col - left][1]
						= ULIM(val, pix[-1][1], pix[1][1]);
					val = ((pix[-fInputWidth][1] + pix[0][fc]
							+ pix[fInputWidth][1]) * 2
						- pix[-2 * fInputWidth][fc] - pix[2 * fInputWidth][fc])
							>> 2;
					rgb[1][row - top][col - left][1] = ULIM(val,
						pix[-fInputWidth][1], pix[fInputWidth][1]);
				}
			}

			/* Interpolate red and blue, and convert to CIELab: */
			for (d = 0; d < 2; d++) {
				for (row = top + 1; row < top + TS - 1
						&& row < fInputHeight - 1; row++) {
					for (col = left + 1; col < left + TS - 1
							&& col < fInputWidth - 1; col++) {
						pix = fImageData + row * fInputWidth + col;
						rix = &rgb[d][row - top][col - left];
						if ((c = 2 - FC(row, col)) == 1) {
							c = FC(row + 1,col);
							val = pix[0][1] + ((pix[-1][2-c] + pix[1][2 - c]
								- rix[-1][1] - rix[1][1] ) >> 1);
							rix[0][2-c] = CLIP(val);
							val = pix[0][1] + ((pix[-fInputWidth][c]
								+ pix[fInputWidth][c]
								- rix[-TS][1] - rix[TS][1] ) >> 1);
						} else {
							val = rix[0][1] + ((pix[-fInputWidth - 1][c]
								+ pix[-fInputWidth + 1][c]
								+ pix[fInputWidth - 1][c]
								+ pix[fInputWidth + 1][c]
								- rix[-TS - 1][1] - rix[-TS + 1][1]
								- rix[TS - 1][1] - rix[TS + 1][1] + 1) >> 2);
						}
						rix[0][c] = CLIP(val);
						c = FC(row, col);
						rix[0][c] = pix[0][c];
						_CameraToCIELab(rix[0], flab);
						for (c = 0; c < 3; c++) {
							lab[d][row - top][col - left][c]
								= int16(64 * flab[c]);
						}
					}
				}
			}

			/* Build homogeneity maps from the CIELab images: */
			memset(homo, 0, 2 * TS * TS);
			for (row = top + 2; row < top+TS-2 && row < fInputHeight; row++) {
				tr = row - top;
				for (col = left + 2; col < left + TS - 2
						&& col < fInputWidth; col++) {
					tc = col - left;
					for (d = 0; d < 2; d++) {
						for (i = 0; i < 4; i++) {
							ldiff[d][i] = ABS(lab[d][tr][tc][0]
									- lab[d][tr][tc+dir[i]][0]);
						}
					}

					leps = MIN(MAX(ldiff[0][0],ldiff[0][1]),
						MAX(ldiff[1][2],ldiff[1][3]));

					for (d = 0; d < 2; d++) {
						for (i = 0; i < 4; i++) {
							if (i >> 1 == d || ldiff[d][i] <= leps) {
								abdiff[d][i] = square(lab[d][tr][tc][1]
										- lab[d][tr][tc+dir[i]][1])
									+ square(lab[d][tr][tc][2]
										- lab[d][tr][tc+dir[i]][2]);
							}
						}
					}

					abeps = MIN(MAX(abdiff[0][0],abdiff[0][1]),
						MAX(abdiff[1][2],abdiff[1][3]));

					for (d=0; d < 2; d++) {
						for (i=0; i < 4; i++) {
							if (ldiff[d][i] <= leps && abdiff[d][i] <= abeps)
								homo[d][tr][tc]++;
						}
					}
				}
			}

			/* Combine the most homogenous pixels for the final result: */
			for (row = top + 3; row < top + TS - 3 && row < fInputHeight - 3;
					row++) {
				tr = row - top;
				for (col = left + 3; col < left + TS - 3
						&& col < fInputWidth - 3; col++) {
					tc = col - left;
					for (d = 0; d < 2; d++) {
						for (hm[d] = 0, i = tr - 1; i <= tr + 1; i++) {
							for (j = tc - 1; j <= tc + 1; j++) {
								hm[d] += homo[d][i][j];
							}
						}
					}
					if (hm[0] != hm[1]) {
						for (c = 0; c < 3; c++) {
							fImageData[row * fInputWidth + col][c]
								= rgb[hm[1] > hm[0]][tr][tc][c];
						}
					} else {
						for (c = 0; c < 3; c++) {
							fImageData[row * fInputWidth + col][c]
								= (rgb[0][tr][tc][c] + rgb[1][tr][tc][c]) >> 1;
						}
					}
				}
			}
		}
	}
	free(buffer);
#undef TS
}


void
DCRaw::_PseudoInverse(double (*in)[3], double (*out)[3], uint32 size)
{
	double work[3][6], num;
	uint32 i, j, k;

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 6; j++) {
			work[i][j] = j == i + 3;
		}
		for (j = 0; j < 3; j++) {
			for (k = 0; k < size; k++) {
				work[i][j] += in[k][i] * in[k][j];
			}
		}
	}

	for (i = 0; i < 3; i++) {
		num = work[i][i];
		for (j = 0; j < 6; j++) {
			work[i][j] /= num;
		}
		for (k = 0; k < 3; k++) {
			if (k == i)
				continue;

			num = work[k][i];

			for (j = 0; j < 6; j++) {
				work[k][j] -= work[i][j] * num;
			}
		}
	}

	for (i = 0; i < size; i++) {
		for (j = 0; j < 3; j++) {
			for (out[i][j] = k =0; k < 3; k++) {
				out[i][j] += work[j][k+3] * in[i][k];
			}
		}
	}
}


void
DCRaw::_ConvertToRGB()
{
	if (fProgressMonitor != NULL)
		fProgressMonitor("Convert to RGB", 90, fProgressData);

	uint32 row, col, c, i, j, k;
	float out[3], out_cam[3][4];
	double num, inverse[3][3];
	static const double xyzd50_srgb[3][3] = {
		{ 0.436083, 0.385083, 0.143055 },
		{ 0.222507, 0.716888, 0.060608 },
		{ 0.013930, 0.097097, 0.714022 }};
	static const double rgb_rgb[3][3] = {
		{ 1,0,0 }, { 0,1,0 }, { 0,0,1 }};
	static const double adobe_rgb[3][3] = {
		{ 0.715146, 0.284856, 0.000000 },
		{ 0.000000, 1.000000, 0.000000 },
		{ 0.000000, 0.041166, 0.958839 }};
	static const double wide_rgb[3][3] = {
		{ 0.593087, 0.404710, 0.002206 },
		{ 0.095413, 0.843149, 0.061439 },
		{ 0.011621, 0.069091, 0.919288 }};
	static const double prophoto_rgb[3][3] = {
		{ 0.529317, 0.330092, 0.140588 },
		{ 0.098368, 0.873465, 0.028169 },
		{ 0.016879, 0.117663, 0.865457 }};
	static const double (*out_rgb[])[3]
		= { rgb_rgb, adobe_rgb, wide_rgb, prophoto_rgb, xyz_rgb };
	static const char *name[] = { "sRGB", "Adobe RGB (1998)", "WideGamut D65",
		"ProPhoto D65", "XYZ" };
	static const unsigned phead[] = { 1024, 0, 0x2100000, 0x6d6e7472,
		0x52474220, 0x58595a20, 0, 0, 0, 0x61637370, 0, 0, 0x6e6f6e65,
		0, 0, 0, 0, 0xf6d6, 0x10000, 0xd32d };
	unsigned pbody[] = { 10,
		0x63707274, 0, 36,	/* cprt */
		0x64657363, 0, 40,	/* desc */
		0x77747074, 0, 20,	/* wtpt */
		0x626b7074, 0, 20,	/* bkpt */
		0x72545243, 0, 14,	/* rTRC */
		0x67545243, 0, 14,	/* gTRC */
		0x62545243, 0, 14,	/* bTRC */
		0x7258595a, 0, 20,	/* rXYZ */
		0x6758595a, 0, 20,	/* gXYZ */
		0x6258595a, 0, 20 };	/* bXYZ */
	static const unsigned pwhite[] = { 0xf351, 0x10000, 0x116cc };
	unsigned pcurve[] = { 0x63757276, 0, 1, 0x1000000 };

	memcpy(out_cam, fMeta.rgb_camera, sizeof(out_cam));
	fRawColor |= fColors == 1 || fDocumentMode
		|| fOutputColor < 1 || fOutputColor > 5;
	if (!fRawColor) {
		fOutputProfile = (uint32 *)calloc(phead[0], 1);
		if (fOutputProfile == NULL)
			throw (status_t)B_NO_MEMORY;

		memcpy(fOutputProfile, phead, sizeof(phead));
		if (fOutputColor == 5)
			fOutputProfile[4] = fOutputProfile[5];

		fOutputProfile[0] = 132 + 12 * pbody[0];
		for (i = 0; i < pbody[0]; i++) {
			fOutputProfile[fOutputProfile[0] / 4]
				= i ? (i > 1 ? 0x58595a20 : 0x64657363) : 0x74657874;
			pbody[i*3+2] = fOutputProfile[0];
			fOutputProfile[0] += (pbody[i*3+3] + 3) & -4;
		}

		memcpy(fOutputProfile + 32, pbody, sizeof(pbody));
		fOutputProfile[pbody[5] / 4 + 2] = strlen(name[fOutputColor - 1]) + 1;
		memcpy((char *)fOutputProfile + pbody[8] + 8, pwhite, sizeof(pwhite));
		if (fOutputBitsPerSample == 8) {
#ifdef SRGB_GAMMA
			pcurve[3] = 0x2330000;
#else
			pcurve[3] = 0x1f00000;
#endif
		}

		for (i = 4; i < 7; i++) {
			memcpy((char *)fOutputProfile + pbody[i * 3 + 2], pcurve,
				sizeof(pcurve));
		}

		_PseudoInverse((double (*)[3])out_rgb[fOutputColor - 1], inverse, 3);

		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++) {
				for (num = k=0; k < 3; k++) {
					num += xyzd50_srgb[i][k] * inverse[j][k];
				}
				fOutputProfile[pbody[j * 3 + 23] / 4 + i + 2]
					= uint32(num * 0x10000 + 0.5);
			}
		}
		for (i = 0; i < phead[0]/4; i++) {
			fOutputProfile[i] = htonl(fOutputProfile[i]);
		}
		strcpy((char *)fOutputProfile + pbody[2] + 8,
			"auto-generated by dcraw");
		strcpy((char *)fOutputProfile + pbody[5] + 12, name[fOutputColor - 1]);

		for (i = 0; i < 3; i++) {
			for (j = 0; j < fColors; j++) {
				for (out_cam[i][j] = k = 0; k < 3; k++) {
					out_cam[i][j] += out_rgb[fOutputColor-1][i][k]
						* fMeta.rgb_camera[k][j];
				}
			}
		}
	}

	if (1/*verbose*/) {
		if (fRawColor)
			fprintf(stderr, "Building histograms...\n");
		else {
			fprintf(stderr, "Converting to %s colorspace...\n",
				name[fOutputColor - 1]);
		}
	}

	ushort* img = fImageData[0];
	memset(fHistogram, 0, sizeof(int32) * 0x2000 * 4);

	for (row = 0; row < fInputHeight; row++) {
		for (col = 0; col < fInputWidth; col++, img += 4) {
			if (!fRawColor) {
				out[0] = out[1] = out[2] = 0;
				for (c = 0; c < fColors; c++) {
					out[0] += out_cam[0][c] * img[c];
					out[1] += out_cam[1][c] * img[c];
					out[2] += out_cam[2][c] * img[c];
				}
				for (c = 0; c < 3; c++) {
					img[c] = CLIP((int)out[c]);
				}
			} else if (fDocumentMode)
				img[0] = img[FC(row, col)];

			for (c = 0; c < fColors; c++) {
				fHistogram[img[c] >> 3][c]++;
			}
		}
	}

	if (fColors == 4 && fOutputColor)
		fColors = 3;
	if (fDocumentMode && fFilters)
		fColors = 1;
}


void
DCRaw::_GammaLookUpTable(uchar* lut)
{
	int32 percent, val, total, i;
	float white = 0, r;

	percent = int32(fInputWidth * fInputHeight * 0.01);
		// 99th percentile white point

	//  if (fuji_width) perc /= 2;
	if (fHighlight)
		percent = 0;

	for (uint32 c = 0; c < fColors; c++) {
		for (val = 0x2000, total = 0; --val > 32;) {
			if ((total += fHistogram[val][c]) > percent)
				break;
		}
		if (white < val)
			white = val;
	}

	white *= 8 / fBrightness;

	for (i = 0; i < 0x10000; i++) {
		r = i / white;
		val = int32(256 * (!fUseGamma ? r :
#ifdef SRGB_GAMMA
			r <= 0.00304 ? r*12.92 : pow(r,2.5/6)*1.055-0.055));
#else
			r <= 0.018 ? r*4.5 : pow(r,0.45)*1.099-0.099));
#endif
		if (val > 255)
			val = 255;
		lut[i] = val;
	}
}


//	#pragma mark - Lossless JPEG


void
DCRaw::_InitDecoder()
{
	memset(fDecodeBuffer, 0, sizeof(decode) * kDecodeBufferCount);
	fFreeDecode = fDecodeBuffer;
}


/*!	Construct a decode tree according the specification in *source.
	The first 16 bytes specify how many codes should be 1-bit, 2-bit
	3-bit, etc.  Bytes after that are the leaf values.

	For example, if the source is

	{ 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,
	  0x04,0x03,0x05,0x06,0x02,0x07,0x01,0x08,0x09,0x00,0x0a,0x0b,0xff  },

	then the code is

	00		0x04
	010		0x03
	011		0x05
	100		0x06
	101		0x02
	1100		0x07
	1101		0x01
	11100		0x08
	11101		0x09
	11110		0x00
	111110		0x0a
	1111110		0x0b
	1111111		0xff
*/
uchar *
DCRaw::_MakeDecoder(const uchar* source, int level)
{
	if (level == 0)
		fDecodeLeaf = 0;

	if ((uint8*)fFreeDecode > (uint8*)fDecodeBuffer
			+ sizeof(decode) * kDecodeBufferCount) {
		fprintf(stderr, "decoder table overflow\n");
		throw (status_t)B_ERROR;
	}

	struct decode* current = fFreeDecode++;

	int i, next;
	for (i = next = 0; i <= fDecodeLeaf && next < 16; ) {
		i += source[next++];
	}

	if (i > fDecodeLeaf) {
		if (level < next) {
			current->branch[0] = fFreeDecode;
			_MakeDecoder(source, level + 1);
			current->branch[1] = fFreeDecode;
			_MakeDecoder(source, level + 1);
		} else
			current->leaf = source[16 + fDecodeLeaf++];
	}

	return (uchar*)source + 16 + fDecodeLeaf;
}


/*!	Not a full implementation of Lossless JPEG, just
	enough to decode Canon, Kodak and Adobe DNG images.
*/
void
DCRaw::_InitDecodeBits()
{
	fDecodeBits = fDecodeBitsRead = 0;
	fDecodeBitsReset = false;
}


/*!	_GetDecodeBits(n) where 0 <= n <= 25 returns an n-bit integer
*/
uint32
DCRaw::_GetDecodeBits(uint32 numBits)
{
	if (numBits == 0 || fDecodeBitsReset)
		return 0;

	while (fDecodeBitsRead < numBits) {
		uint8 c = fRead.Next<uint8>();
		if ((fDecodeBitsReset = fDecodeBitsZeroAfterMax
				&& c == 0xff && fRead.Next<uint8>()))
			return 0;
		fDecodeBits = (fDecodeBits << 8) + c;
		fDecodeBitsRead += 8;
	}

	fDecodeBitsRead -= numBits;

	return fDecodeBits << (32 - numBits - fDecodeBitsRead) >> (32 - numBits);
}


status_t
DCRaw::_LosslessJPEGInit(struct jhead* jh, bool infoOnly)
{
	int i, tag, len;

	_InitDecoder();

	for (i = 0; i < 4; i++) {
		jh->huff[i] = fFreeDecode;
	}

	jh->restart = INT_MAX;

	uchar data[0x10000], *dp;
	fRead(data, 2);
	if (data[1] != 0xd8)
		return B_ERROR;

	do {
		fRead(data, 4);
		tag = data[0] << 8 | data[1];
		len = (data[2] << 8 | data[3]) - 2;
		if (tag <= 0xff00)
			return B_ERROR;

		fRead(data, len);
		switch (tag) {
			case 0xffc0:
			case 0xffc3:
				jh->bits = data[0];
				jh->high = data[1] << 8 | data[2];
				jh->wide = data[3] << 8 | data[4];
				jh->clrs = data[5];
				break;
			case 0xffc4:
				if (infoOnly)
					break;

				for (dp = data; dp < data+len && *dp < 4; ) {
					jh->huff[*dp] = fFreeDecode;
					dp = _MakeDecoder(++dp, 0);
				}
				break;
			case 0xffdd:
				jh->restart = data[0] << 8 | data[1];
				break;
		}
	} while (tag != 0xffda);

	if (infoOnly)
		return B_OK;

	jh->row = (ushort *)calloc(jh->wide*jh->clrs, 2);
	if (jh->row == NULL)
		throw (status_t)B_NO_MEMORY;

	fDecodeBitsZeroAfterMax = true;
	return B_OK;
}


int
DCRaw::_LosslessJPEGDiff(struct decode *dindex)
{
	while (dindex->branch[0]) {
		dindex = dindex->branch[_GetDecodeBits(1)];
	}

	int length = dindex->leaf;
	if (length == 16 && (!fDNGVersion || fDNGVersion >= 0x1010000))
		return -32768;

	int diff = _GetDecodeBits(length);
	if ((diff & (1 << (length - 1))) == 0)
		diff -= (1 << length) - 1;

	return diff;
}


void
DCRaw::_LosslessJPEGRow(struct jhead *jh, int jrow)
{
	if (jrow * jh->wide % jh->restart == 0) {
		for (uint32 i = 0; i < 4; i++) {
			jh->vpred[i] = 1 << (jh->bits - 1);
		}
		if (jrow) {
			uint16 mark = 0;
			int c;
			do {
				mark = (mark << 8) + (c = fRead.Next<uint8>());
			} while (c != EOF && mark >> 4 != 0xffd);
		}
		_InitDecodeBits();
	}

	uint16* outp = jh->row;

	for (int32 col = 0; col < jh->wide; col++) {
		for (int32 c = 0; c < jh->clrs; c++) {
			int32 diff = _LosslessJPEGDiff(jh->huff[c]);
			*outp = col ? outp[-jh->clrs]+diff : (jh->vpred[c] += diff);
			outp++;
		}
	}
}


//	#pragma mark - RAW loaders


void
DCRaw::_LoadRAWUnpacked(const image_data_info& image)
{
	uint32 rawWidth = _Raw().width;

	uint16* pixel = (uint16*)calloc(rawWidth, sizeof(uint16));
	if (pixel == NULL)
		return;

	fRead.Seek((fTopMargin * rawWidth + fLeftMargin) * sizeof(uint16),
		SEEK_CUR);

	for (uint32 row = 0; row < fInputHeight; row++) {
		fRead.NextShorts(pixel, rawWidth);
		for (uint32 column = 0; column < fInputWidth; column++) {
			_Bayer(column, row) = pixel[column];
		}
	}

	free(pixel);
}


/*!	This is, for example, used in PENTAX RAW images
*/
void
DCRaw::_LoadRAWPacked12(const image_data_info& image)
{
	uint32 rawWidth = _Raw().width;

	_InitDecodeBits();

	for (uint32 row = 0; row < fInputHeight; row++) {
		for (uint32 column = 0; column < fInputWidth; column++) {
			//uint16 bits = _GetDecodeBits(12);
			_Bayer(column, row) = _GetDecodeBits(12);
			//fImageData[((row) >> fShrink)*fOutputWidth + ((column) >> fShrink)][FC(row,column)] = bits;
		}
		for (uint32 column = fInputWidth * 3 / 2; column < rawWidth; column++) {
			_GetDecodeBits(8);
		}
	}
}


void
DCRaw::_MakeCanonDecoder(uint32 table)
{
	static const uchar kFirstTree[3][29] = {
		{ 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,
			0x04,0x03,0x05,0x06,0x02,0x07,0x01,0x08,0x09,0x00,0x0a,0x0b,0xff },
		{ 0,2,2,3,1,1,1,1,2,0,0,0,0,0,0,0,
			0x03,0x02,0x04,0x01,0x05,0x00,0x06,0x07,0x09,0x08,0x0a,0x0b,0xff },
		{ 0,0,6,3,1,1,2,0,0,0,0,0,0,0,0,0,
			0x06,0x05,0x07,0x04,0x08,0x03,0x09,0x02,0x00,0x0a,0x01,0x0b,0xff },
	};
	static const uchar kSecondTree[3][180] = {
		{ 0,2,2,2,1,4,2,1,2,5,1,1,0,0,0,139,
			0x03,0x04,0x02,0x05,0x01,0x06,0x07,0x08,
			0x12,0x13,0x11,0x14,0x09,0x15,0x22,0x00,0x21,0x16,0x0a,0xf0,
			0x23,0x17,0x24,0x31,0x32,0x18,0x19,0x33,0x25,0x41,0x34,0x42,
			0x35,0x51,0x36,0x37,0x38,0x29,0x79,0x26,0x1a,0x39,0x56,0x57,
			0x28,0x27,0x52,0x55,0x58,0x43,0x76,0x59,0x77,0x54,0x61,0xf9,
			0x71,0x78,0x75,0x96,0x97,0x49,0xb7,0x53,0xd7,0x74,0xb6,0x98,
			0x47,0x48,0x95,0x69,0x99,0x91,0xfa,0xb8,0x68,0xb5,0xb9,0xd6,
			0xf7,0xd8,0x67,0x46,0x45,0x94,0x89,0xf8,0x81,0xd5,0xf6,0xb4,
			0x88,0xb1,0x2a,0x44,0x72,0xd9,0x87,0x66,0xd4,0xf5,0x3a,0xa7,
			0x73,0xa9,0xa8,0x86,0x62,0xc7,0x65,0xc8,0xc9,0xa1,0xf4,0xd1,
			0xe9,0x5a,0x92,0x85,0xa6,0xe7,0x93,0xe8,0xc1,0xc6,0x7a,0x64,
			0xe1,0x4a,0x6a,0xe6,0xb3,0xf1,0xd3,0xa5,0x8a,0xb2,0x9a,0xba,
			0x84,0xa4,0x63,0xe5,0xc5,0xf3,0xd2,0xc4,0x82,0xaa,0xda,0xe4,
			0xf2,0xca,0x83,0xa3,0xa2,0xc3,0xea,0xc2,0xe2,0xe3,0xff,0xff },
		{ 0,2,2,1,4,1,4,1,3,3,1,0,0,0,0,140,
			0x02,0x03,0x01,0x04,0x05,0x12,0x11,0x06,
			0x13,0x07,0x08,0x14,0x22,0x09,0x21,0x00,0x23,0x15,0x31,0x32,
			0x0a,0x16,0xf0,0x24,0x33,0x41,0x42,0x19,0x17,0x25,0x18,0x51,
			0x34,0x43,0x52,0x29,0x35,0x61,0x39,0x71,0x62,0x36,0x53,0x26,
			0x38,0x1a,0x37,0x81,0x27,0x91,0x79,0x55,0x45,0x28,0x72,0x59,
			0xa1,0xb1,0x44,0x69,0x54,0x58,0xd1,0xfa,0x57,0xe1,0xf1,0xb9,
			0x49,0x47,0x63,0x6a,0xf9,0x56,0x46,0xa8,0x2a,0x4a,0x78,0x99,
			0x3a,0x75,0x74,0x86,0x65,0xc1,0x76,0xb6,0x96,0xd6,0x89,0x85,
			0xc9,0xf5,0x95,0xb4,0xc7,0xf7,0x8a,0x97,0xb8,0x73,0xb7,0xd8,
			0xd9,0x87,0xa7,0x7a,0x48,0x82,0x84,0xea,0xf4,0xa6,0xc5,0x5a,
			0x94,0xa4,0xc6,0x92,0xc3,0x68,0xb5,0xc8,0xe4,0xe5,0xe6,0xe9,
			0xa2,0xa3,0xe3,0xc2,0x66,0x67,0x93,0xaa,0xd4,0xd5,0xe7,0xf8,
			0x88,0x9a,0xd7,0x77,0xc4,0x64,0xe2,0x98,0xa5,0xca,0xda,0xe8,
			0xf3,0xf6,0xa9,0xb2,0xb3,0xf2,0xd2,0x83,0xba,0xd3,0xff,0xff },
		{ 0,0,6,2,1,3,3,2,5,1,2,2,8,10,0,117,
			0x04,0x05,0x03,0x06,0x02,0x07,0x01,0x08,
			0x09,0x12,0x13,0x14,0x11,0x15,0x0a,0x16,0x17,0xf0,0x00,0x22,
			0x21,0x18,0x23,0x19,0x24,0x32,0x31,0x25,0x33,0x38,0x37,0x34,
			0x35,0x36,0x39,0x79,0x57,0x58,0x59,0x28,0x56,0x78,0x27,0x41,
			0x29,0x77,0x26,0x42,0x76,0x99,0x1a,0x55,0x98,0x97,0xf9,0x48,
			0x54,0x96,0x89,0x47,0xb7,0x49,0xfa,0x75,0x68,0xb6,0x67,0x69,
			0xb9,0xb8,0xd8,0x52,0xd7,0x88,0xb5,0x74,0x51,0x46,0xd9,0xf8,
			0x3a,0xd6,0x87,0x45,0x7a,0x95,0xd5,0xf6,0x86,0xb4,0xa9,0x94,
			0x53,0x2a,0xa8,0x43,0xf5,0xf7,0xd4,0x66,0xa7,0x5a,0x44,0x8a,
			0xc9,0xe8,0xc8,0xe7,0x9a,0x6a,0x73,0x4a,0x61,0xc7,0xf4,0xc6,
			0x65,0xe9,0x72,0xe6,0x71,0x91,0x93,0xa6,0xda,0x92,0x85,0x62,
			0xf3,0xc5,0xb2,0xa4,0x84,0xba,0x64,0xa5,0xb3,0xd2,0x81,0xe5,
			0xd3,0xaa,0xc4,0xca,0xf2,0xb1,0xe4,0xd1,0x83,0x63,0xea,0xc3,
			0xe2,0x82,0xf1,0xa3,0xc2,0xa1,0xc1,0xe3,0xa2,0xe1,0xff,0xff }
	};

	if (table > 2)
		table = 2;

	_InitDecoder();

	_MakeDecoder(kFirstTree[table], 0);
	fSecondDecode = fFreeDecode;
	_MakeDecoder(kSecondTree[table], 0);
}


/*!	Return 0 if the image starts with compressed data,
	1 if it starts with uncompressed low-order bits.

	In Canon compressed data, 0xff is always followed by 0x00.
*/
bool
DCRaw::_CanonHasLowBits()
{
	bool hasLowBits = true;
	uchar test[0x4000 - 540];

	fRead.Seek(540, SEEK_SET);
	fRead(test, sizeof(test));

	for (uint32 i = 0; i < sizeof(test) - 1; i++)
		if (test[i] == 0xff) {
			if (test[i + 1])
				return 1;
			hasLowBits = 0;
    }

	return hasLowBits;
}


void
DCRaw::_LoadRAWCanonCompressed(const image_data_info& image)
{
	uint32 rawWidth = _Raw().width;
	int carry = 0, pnum = 0, base[2];

	_MakeCanonDecoder(image.compression);

	uint16* pixel = (uint16 *)calloc(rawWidth * 8, sizeof(*pixel));
	if (pixel == NULL)
		throw (status_t)B_NO_MEMORY;

	bool hasLowBits = _CanonHasLowBits();
	if (!hasLowBits)
		fMeta.maximum = 0x3ff;

	fRead.Seek(540 + (hasLowBits ? _Raw().height * rawWidth / 4 : 0),
		SEEK_SET);

	fDecodeBitsZeroAfterMax = true;
	_InitDecodeBits();

	for (uint32 row = 0; row < _Raw().height; row += 8) {
		for (uint32 block = 0; block < rawWidth >> 3; block++) {
			int diffbuf[64];
			memset(diffbuf, 0, sizeof diffbuf);
			struct decode* decode = fDecodeBuffer;

			for (uint32 i = 0; i < 64; i++) {
				struct decode* dindex = decode;
				while (dindex->branch[0]) {
					dindex = dindex->branch[_GetDecodeBits(1)];
				}
				int leaf = dindex->leaf;
				decode = fSecondDecode;
				if (leaf == 0 && i)
					break;
				if (leaf == 0xff)
					continue;
				i += leaf >> 4;

				int len = leaf & 15;
				if (len == 0)
					continue;
				int diff = _GetDecodeBits(len);
				if ((diff & (1 << (len-1))) == 0)
					diff -= (1 << len) - 1;
				if (i < 64)
					diffbuf[i] = diff;
			}

			diffbuf[0] += carry;
			carry = diffbuf[0];

			for (uint32 i = 0; i < 64; i++) {
				if (pnum++ % _Raw().width == 0)
					base[0] = base[1] = 512;
				pixel[(block << 6) + i] = (base[i & 1] += diffbuf[i]);
			}
		}

		if (hasLowBits) {
			off_t savedOffset = fRead.Position();
			fRead.Seek(26 + row * _Raw().width / 4, SEEK_SET);

			uint16* pixelRow = pixel;
			for (uint32 i = 0; i < rawWidth * 2; i++) {
				uint8 c = fRead.Next<uint8>();

				for (uint32 r = 0; r < 8; r += 2, pixelRow++) {
					uint32 val = (*pixelRow << 2) + ((c >> r) & 3);
					if (rawWidth == 2672 && val < 512)
						val += 2;
					*pixelRow = val;
				}
			}

			fRead.Seek(savedOffset, SEEK_SET);
		}

		for (uint32 r = 0; r < 8; r++) {
			uint32 irow = row - fTopMargin + r;
			if (irow >= fInputHeight)
				continue;

			for (uint32 col = 0; col < rawWidth; col++) {
				uint32 icol = col - fLeftMargin;
				if (icol < fInputWidth)
					_Bayer(icol, irow) = pixel[r * rawWidth + col];
				else
					fMeta.black += pixel[r * rawWidth + col];
			}
		}
	}

	free(pixel);

	if (rawWidth > fInputWidth)
		fMeta.black /= (rawWidth - fInputWidth) * fInputHeight;
}


void
DCRaw::_LoadRAWLosslessJPEG(const image_data_info& image)
{
	int jwide, jrow, jcol, val, jidx, i, j, row = 0, col = 0;
	uint32 rawWidth = _Raw().width;
	int min = INT_MAX;

	struct jhead jh;
	if (_LosslessJPEGInit(&jh, false) != B_OK)
		throw (status_t)B_NO_TRANSLATOR;

	jwide = jh.wide * jh.clrs;

	for (jrow = 0; jrow < jh.high; jrow++) {
		_LosslessJPEGRow(&jh, jrow);

		for (jcol = 0; jcol < jwide; jcol++) {
			val = jh.row[jcol];
			if (jh.bits <= 12)
				val = fCurve[val];

			if (fCR2Slice[0]) {
				jidx = jrow * jwide + jcol;
				i = jidx / (fCR2Slice[1] * jh.high);
				if ((j = i >= fCR2Slice[0]))
					i  = fCR2Slice[0];
				jidx -= i * (fCR2Slice[1] * jh.high);
				row = jidx / fCR2Slice[1 + j];
				col = jidx % fCR2Slice[1 + j] + i * fCR2Slice[1];
			}

			if (_Raw().width == 3984 && (col -= 2) < 0) {
				col += rawWidth;
				row--;
			}

			if (uint32(row - fTopMargin) < fInputHeight) {
				if (uint32(col - fLeftMargin) < fInputWidth) {
					_Bayer(col - fLeftMargin, row - fTopMargin) = val;
					if (min > val)
						min = val;
				} else
					fMeta.black += val;
			}
			if (++col >= (int32)rawWidth) {
				col = 0;
				row++;
			}
		}
	}

	//dump_to_disk(fImageData, fInputWidth * fColors * 100);
	free(jh.row);

	if (rawWidth > fInputWidth)
		fMeta.black /= (rawWidth - fInputWidth) * fInputHeight;
	if (_IsKodak())
		fMeta.black = min;
}


void
DCRaw::_LoadRAW(const image_data_info& image)
{
#if 0
	if (_IsCanon()) {
		if (fIsTIFF)
		else
			_LoadRAWCanonCompressed(image);
	} else
#endif
	{
		switch (image.compression) {
			case COMPRESSION_NONE:
				_LoadRAWUnpacked(image);
				break;
			case COMPRESSION_OLD_JPEG:
				_LoadRAWLosslessJPEG(image);
				//_LoadRAWCanonCompressed(image);
				break;
			case COMPRESSION_PACKBITS:
				_LoadRAWPacked12(image);
				break;

			default:
				fprintf(stderr, "DCRaw: unknown compression: %ld\n",
					image.compression);
				throw (status_t)B_NO_TRANSLATOR;
				break;
		}
	}
}


//	#pragma mark - Image writers


void
DCRaw::_WriteRGB32(image_data_info& image, uint8* outputBuffer)
{
	if (fProgressMonitor != NULL)
		fProgressMonitor("Write RGB", 95, fProgressData);

	uint8* line, lookUpTable[0x10000];

	uint32 width = image.flip > 4 ? fOutputHeight : fOutputWidth;
	uint32 height = image.flip > 4 ? fOutputWidth : fOutputHeight;
	uint32 outputRow = (4 * fOutputBitsPerSample / 8) * width;
	uint32 outputOffset = 0;

	line = (uint8 *)malloc(outputRow);
	if (line == NULL)
		throw (status_t)B_NO_MEMORY;

	memset(line, 0, outputRow);

	if (fOutputBitsPerSample == 8)
		_GammaLookUpTable(lookUpTable);

	int32 sourceOffset = _FlipIndex(0, 0, image.flip);
	int32 colStep = _FlipIndex(0, 1, image.flip) - sourceOffset;
	int32 rowStep = _FlipIndex(1, 0, image.flip)
		- _FlipIndex(0, width, image.flip);

	TRACE(("flip = %ld, sourceOffset = %ld, colStep = %ld, rowStep = %ld, "
		"input: %lu x %lu, output: %lu x %lu\n", image.flip, sourceOffset,
		colStep, rowStep, fInputWidth, fInputHeight, width,
		height));

	if (fOutputBitsPerSample == 8) {
		for (uint32 row = 0; row < height; row++, sourceOffset += rowStep) {
			for (uint32 col = 0; col < width; col++, sourceOffset += colStep) {
				line[col * 4 + 2] = lookUpTable[fImageData[sourceOffset][0]];
				line[col * 4 + 1] = lookUpTable[fImageData[sourceOffset][1]];
				line[col * 4 + 0] = lookUpTable[fImageData[sourceOffset][2]];
			}

			memcpy(&outputBuffer[outputOffset], line, outputRow);
			outputOffset += outputRow;
		}
	} else {
#if 0
		uint16* ppm2 = (uint16*)line;
		for (row = 0; row < fOutputHeight; row++, soff += rstep) {
			for (col = 0; col < fOutputWidth; col++, soff += cstep) {
				FORCC ppm2[col*colors+c] =     image[soff][c];
			}
			if (!output_tiff && htons(0x55aa) != 0x55aa)
				swab (ppm2, ppm2, width*colors*2);
			fwrite (ppm, colors*output_bps/8, width, ofp);
		}
#endif
	}

	free(line);
}


void
DCRaw::_WriteJPEG(image_data_info& image, uint8* outputBuffer)
{
	fRead(outputBuffer, image.bytes);

	if (outputBuffer[0] != 0xff || outputBuffer[1] != 0xd8)
		throw (status_t)B_NO_TRANSLATOR;

#if 0
	uint8* thumb = (uint8*)malloc(image.bytes);
	if (thumb == NULL)
		throw (status_t)B_NO_MEMORY;

	fRead(thumb, image.bytes);

	uint8* data = (uint8*)fImageData;
	data[0] = 0xff;
	data[1] = 0xd8;

	if (strcmp((char *)thumb + 6, "Exif")) {
		// TODO: no EXIF data - write them ourselves
	}

	memcpy(&data[2], thumb + 2, image.bytes - 2);
	free(thumb);
#endif
}


//	#pragma mark - TIFF


time_t
DCRaw::_ParseTIFFTimestamp(bool reversed)
{
	char str[20];
	str[19] = 0;

	if (reversed) {
		for (int i = 19; i--; ) {
			str[i] = fRead.Next<uint8>();
		}
	} else
		fRead(str, 19);

	struct tm t;
	memset(&t, 0, sizeof t);

	if (sscanf(str, "%d:%d:%d %d:%d:%d", &t.tm_year, &t.tm_mon,
			&t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) != 6)
		return 0;

	t.tm_year -= 1900;
	t.tm_mon -= 1;

	return mktime(&t);
}


/*!	Reads a TIFF tag and positions the file stream to its data section
*/
void
DCRaw::_ParseTIFFTag(off_t baseOffset, tiff_tag& tag, off_t& offset)
{
	fRead(tag.tag);
	fRead(tag.type);
	fRead(tag.length);

	offset = fRead.Position() + 4;

	uint32 length = tag.length;

	switch (tag.type) {
		case TIFF_UINT16_TYPE:
		case TIFF_INT16_TYPE:
			length *= 2;
			break;

		case TIFF_UINT32_TYPE:
		case TIFF_INT32_TYPE:
		case TIFF_FLOAT_TYPE:
			length *= 4;
			break;

		case TIFF_UFRACTION_TYPE:
		case TIFF_FRACTION_TYPE:
		case TIFF_DOUBLE_TYPE:
			length *= 8;
			break;

		default:
			break;
	}

	if (length > 4) {
		uint32 position;
		fRead(position);

		fRead.Seek(baseOffset + position, SEEK_SET);
	}
}


status_t
DCRaw::_ParseTIFFImageFileDirectory(off_t baseOffset, uint32 offset)
{
	double analogBalance[] = {1, 1, 1, 1};
	double xyz[] = {1, 1, 1, 1};
	bool useColorMatrix = false;
	double cameraCalibration[4][4], colorMatrix[4][3], cameraXYZ[4][3];

	for (int32 j = 0; j < 4; j++) {
		for (int32 i = 0; i < 4; i++) {
			cameraCalibration[j][i] = i == j;
		}
	}

	fRead.Seek(baseOffset + offset, SEEK_SET);

	uint16 tags;
	fRead(tags);
	if (tags > 512)
		return B_BAD_DATA;

	image_data_info& image = fImages[fNumImages];

	while (tags--) {
		off_t nextOffset;
		tiff_tag tag;
		_ParseTIFFTag(baseOffset, tag, nextOffset);
		TAG("TIFF tag: %u\n", tag.tag);

		switch (tag.tag) {
#if 0
			default:
				printf("tag %u NOT HANDLED!\n", tag.tag);
				break;
#endif

			case 17:
			case 18:
				if (tag.type == 3 && tag.length == 1) {
					fMeta.camera_multipliers[(tag.tag - 17) * 2]
						= fRead.Next<uint16>() / 256.0;
				}
				break;

			case 23:	// ISO speed
				fMeta.iso_speed = fRead.Next(tag.type);
				break;

			case 36:
			case 37:
			case 38:
				fMeta.camera_multipliers[tag.tag - 0x24] = fRead.Next<uint16>();
				break;

			case 39:
				if (tag.length < 50 || fMeta.camera_multipliers[0])
					break;

				fRead.Stream().Seek(12, SEEK_CUR);
				for (uint32 i = 0; i < 3; i++) {
					fMeta.camera_multipliers[i] = fRead.Next<uint16>();
				}
				break;

			case 2:		// image width
			case 256:
				image.width = fRead.Next(tag.type);
				break;

			case 3:		// image height
			case 257:
				image.height = fRead.Next(tag.type);
				break;

			case 258:	// bits per sample
				image.samples = tag.length;
				image.bits_per_sample = fRead.Next<uint16>();
				break;

			case 259:	// compression
				image.compression = fRead.Next<uint16>();
				break;

			case 262:	// Photometric Interpretation
				image.photometric_interpretation = fRead.Next<uint16>();
				break;

			case 271:	// manufacturer
				fRead(fMeta.manufacturer, 64);
				break;

			case 272:	// model
				fRead(fMeta.model, 64);
				break;

			case 273:	// Strip Offset
			case 513:
				image.data_offset = baseOffset + fRead.Next<uint32>();
				if (!image.bits_per_sample) {
					fRead.Stream().Seek(image.data_offset, SEEK_SET);
					jhead jh;
					if (_LosslessJPEGInit(&jh, true) == B_OK) {
						image.compression = 6;
						image.width = jh.wide << (jh.clrs == 2);
						image.height = jh.high;
						image.bits_per_sample = jh.bits;
						image.samples = jh.clrs;
					}
				}
				break;

			case 274:	// Orientation
				image.flip = fRead.Next<uint16>();
				break;

			case 277:	// Samples Per Pixel
				image.samples = fRead.Next(tag.type);
				break;

			case 279:	// Strip Byte Counts
			case 514:
				image.bytes = fRead.Next<uint32>();
				break;

			case 305:	// Software
				fRead(fMeta.software, 64);
				if (!strncmp(fMeta.software, "Adobe", 5)
					|| !strncmp(fMeta.software, "dcraw", 5)
					|| !strncmp(fMeta.software, "Bibble", 6)
					|| !strncmp(fMeta.software, "Nikon Scan", 10)
					|| !strcmp(fMeta.software,"Digital Photo Professional"))
					throw (status_t)B_NO_TRANSLATOR;
				break;

			case 306:	// Date/Time
				fMeta.timestamp = _ParseTIFFTimestamp(false);
				break;

#if 0
			case 323:	// Tile Length
				tile_length = fRead.Next(type);
				break;

			case 324:	// Tile Offsets
				image.data_offset = tag.length > 1
					? fRead.Stream().Position() : fRead.Next<uint32>();
				if (tag.length == 4)
					load_raw = &CLASS sinar_4shot_load_raw;
				break;
#endif

			case 330:	// Sub IFDs
				if (!strcmp(fMeta.model, "DSLR-A100") && image.width == 3872) {
					// TODO: this might no longer work!
					image.data_offset = fRead.Next<uint32>() + baseOffset;
					break;
				}

				while (tag.length--) {
					off_t nextOffset = fRead.Position() + sizeof(uint32);

					fRead.Seek(fRead.Next<uint32>() + baseOffset, SEEK_SET);
					if (_ParseTIFFImageFileDirectory(baseOffset) != B_OK)
						break;

					fNumImages++;
					fRead.Seek(nextOffset, SEEK_SET);
				}
				break;

#if 0
			case 400:
				strcpy(fMeta.manufacturer, "Sarnoff");
				maximum = 0xfff;
				break;
#endif

#if 0
			case 29184:
				sony_offset = get4();
				break;
			case 29185:
				sony_length = get4();
				break;
			case 29217:
				sony_key = get4();
				break;
#endif

			case 29443:
				for (uint32 i = 0; i < 4; i++) {
					fMeta.camera_multipliers[i ^ (i < 2)] = fRead.Next<uint16>();
				}
				break;

			case 33405:	// Model 2
				fRead(fMeta.model + 64, 64);
				break;

#if 0
			case 33422:	// CFA Pattern
			case 64777:	// Kodak P-series
			{
				if ((plen=len) > 16) plen = 16;
				fread (cfa_pat, 1, plen, ifp);
				for (colors=cfa=i=0; i < plen; i++) {
				  colors += !(cfa & (1 << cfa_pat[i]));
				  cfa |= 1 << cfa_pat[i];
				}
				if (cfa == 070) memcpy (cfa_pc,"\003\004\005",3);	/* CMY */
				if (cfa == 072) memcpy (cfa_pc,"\005\003\004\001",4);	/* GMCY */
				goto guess_cfa_pc;
				break;
			}

			case 33424:
				fseek(ifp, get4()+base, SEEK_SET);
				parse_kodak_ifd (base);
				break;
#endif

			case 33434:	// Exposure Time
				fMeta.shutter = fRead.NextDouble(TIFF_FRACTION_TYPE);
				break;

			case 33437:	// Aperture
				fMeta.aperture = fRead.NextDouble(TIFF_FRACTION_TYPE);
				break;

			case 34306:	// Leaf white balance
				for (uint32 i = 0; i < 4; i++) {
					fMeta.camera_multipliers[i ^ 1] = 4096.0 / fRead.Next<uint16>();
				}
				break;

#if 0
			case 34307:	// Leaf Catch Light color matrix
				fread (software, 1, 7, ifp);
				if (strncmp(software,"MATRIX",6))
					break;
				colors = 4;
				for (fRawColor = i=0; i < 3; i++) {
					FORC4 fscanf (ifp, "%f", &rgb_cam[i][c^1]);
					if (!use_camera_wb)
						continue;
					num = 0;
					FORC4 num += rgb_cam[i][c];
					FORC4 rgb_cam[i][c] /= num;
				}
				break;
			case 34310:	// Leaf metadata
				parse_mos (ftell(ifp));
			case 34303:
				strcpy(image.manufacturer, "Leaf");
				break;
#endif

			case 34665:	// EXIF tag
				fRead.Seek(fRead.Next<uint32>() + baseOffset, SEEK_SET);

				fEXIFOffset = fRead.Position();
				fEXIFLength = tag.length;

				_ParseEXIF(baseOffset);
				break;

#if 0
			case 34675:	// InterColorProfile
			case 50831:	// AsShotICCProfile
				profile_offset = fRead.Stream().Position();
				profile_length = tag.length;
				break;

			case 37122:	// Compressed Bits Per Pixel
				kodak_cbpp = fRead.Next<uint32>();
				break;
#endif

			case 37386:	// Focal Length
				fMeta.focal_length = fRead.NextDouble(TIFF_FRACTION_TYPE);
				break;

			case 37393:	// Image Number
				fMeta.shot_order = fRead.Next(tag.type);
				break;

#if 0
			case 37400:	// old Kodak KDC tag
				for (fRawColor = i=0; i < 3; i++) {
					getrat();
					FORC3 rgb_cam[i][c] = getrat();
				}
				break;

			case 46275:	// Imacon tags
				strcpy (make, "Imacon");
				data_offset = ftell(ifp);
				ima_len = len;
				break;
      case 46279:
	fseek (ifp, 78, SEEK_CUR);
	raw_width  = get4();
	raw_height = get4();
	left_margin = get4() & 7;
	width = raw_width - left_margin - (get4() & 7);
	top_margin = get4() & 7;
	height = raw_height - top_margin - (get4() & 7);
	fseek (ifp, 52, SEEK_CUR);
	FORC3 cam_multipliers[c] = getreal(11);
	fseek (ifp, 114, SEEK_CUR);
	flip = (get2() >> 7) * 90;
	if (width * height * 6 == ima_len) {
	  if (flip % 180 == 90) SWAP(width,height);
	  filters = flip = 0;
	}
	break;
      case 50454:			/* Sinar tag */
      case 50455:
	if (!(cbuf = (char *) malloc(len))) break;
	fread (cbuf, 1, len, ifp);
	for (cp = cbuf-1; cp && cp < cbuf+len; cp = strchr(cp,'\n'))
	  if (!strncmp (++cp,"Neutral ",8))
	    sscanf (cp+8, "%f %f %f", cam_multipliers, cam_multipliers+1, cam_multipliers+2);
	free (cbuf);
	break;
#endif

			case 50706:	// DNG Version
				for (int32 i = 0; i < 4; i++) {
					fDNGVersion = (fDNGVersion << 8) + fRead.Next<uint8>();
				}
				break;

#if 0
			case 50710:	// CFAPlaneColor
				if (len > 4)
					len = 4;
				colors = len;
				fread(cfa_pc, 1, colors, ifp);
			guess_cfa_pc:
				FORCC tab[cfa_pc[c]] = c;
				cdesc[c] = 0;
				for (i=16; i--; )
					filters = filters << 2 | tab[cfa_pat[i % plen]];
				break;
			case 50711:	// CFALayout
				if (get2() == 2) {
					fuji_width = 1;
					filters = 0x49494949;
				}
				break;
#endif

			case 291:	// Linearization Table
			case 50712:
				_ParseLinearTable(tag.length);
				break;

			case 50714:			/* BlackLevel */
			case 50715:			/* BlackLevelDeltaH */
			case 50716:			/* BlackLevelDeltaV */
			{
				double black = 0.0;
				for (uint32 i = 0; i < tag.length; i++) {
					black += fRead.NextDouble(tag.type);
				}
				fMeta.black += int32(black / tag.length + 0.5);
				break;
			}

			case 50717:	// White Level
				fMeta.maximum = fRead.Next(tag.type);
				break;

			case 50718:	// Default Scale
				fMeta.pixel_aspect = fRead.NextDouble(TIFF_FRACTION_TYPE);
				fMeta.pixel_aspect /= fRead.NextDouble(TIFF_FRACTION_TYPE);
				break;

			case 50721:	// Color Matrix
			case 50722:
				for (uint32 c = 0; c < fColors; c++) {
					for (uint32 j = 0; j < 3; j++) {
						colorMatrix[c][j] = fRead.NextDouble(TIFF_FRACTION_TYPE);
					}
				}
				useColorMatrix = true;
				break;

			case 50723:	// Camera Calibration
			case 50724:
				for (uint32 i = 0; i < fColors; i++) {
					for (uint32 c = 0; c < fColors; c++) {
						cameraCalibration[i][c] = fRead.NextDouble(
							TIFF_FRACTION_TYPE);
					}
				}
				//break;
			case 50727:	// Analog Balance
				for (uint32 c = 0; c < fColors; c++) {
					analogBalance[c] = fRead.NextDouble(TIFF_FRACTION_TYPE);
					//printf("ab: %g\n", analogBalance[c]);
				}
				break;
#if 0
      case 50728:			/* AsShotNeutral */
	FORCC asn[c] = getreal(type);
	break;
      case 50729:			/* AsShotWhiteXY */
	xyz[0] = getrat();
	xyz[1] = getrat();
	xyz[2] = 1 - xyz[0] - xyz[1];
	FORC3 xyz[c] /= kD65White[c];
	break;
      case 50740:			/* DNGPrivateData */
	if (dng_version) break;
	i = order;
	parse_minolta (j = get4()+base);
	order = i;
	fseek (ifp, j, SEEK_SET);
	parse_tiff_ifd (base);
	break;
#endif
			case 50752:
				fRead.NextShorts(fCR2Slice, 3);
				break;

			case 50829:	// Active Area
				fTopMargin = fRead.Next(tag.type);
				fLeftMargin = fRead.Next(tag.type);
				fInputHeight = fRead.Next(tag.type) - fTopMargin;
				fInputWidth = fRead.Next(tag.type) - fLeftMargin;
				break;
#if 0
      case 64772:			/* Kodak P-series */
	fseek (ifp, 16, SEEK_CUR);
	data_offset = get4();
	fseek (ifp, 28, SEEK_CUR);
	data_offset += get4();
	load_raw = &CLASS packed_12_load_raw;
#endif
		}
		fRead.Seek(nextOffset, SEEK_SET);
	}

	// handle SONY tags

#if 0
	if (sony_length && (buf = (unsigned *) malloc(sony_length))) {
		fseek(ifp, sony_offset, SEEK_SET);
		fread(buf, sony_length, 1, ifp);
		sony_decrypt(buf, sony_length / 4, 1, sony_key);
		sfp = ifp;
		if ((ifp = tmpfile())) {
			fwrite(buf, sony_length, 1, ifp);
			fseek(ifp, 0, SEEK_SET);
			parse_tiff_ifd(-sony_offset);
			fclose(ifp);
		}
		ifp = sfp;
		free(buf);
	}
#endif

	for (uint32 i = 0; i < fColors; i++) {
		for (uint32 c = 0; c < fColors; c++) {
			cameraCalibration[i][c] *= analogBalance[i];
		}
	}

	if (useColorMatrix) {
		for (uint32 c = 0; c < fColors; c++) {
			for (uint32 i = 0; i < 3; i++) {
				cameraXYZ[c][i] = 0;
				for (uint32 j = 0; j < fColors; j++) {
					cameraXYZ[c][i] += cameraCalibration[c][j]
						* colorMatrix[j][i] * xyz[i];
				}
			}
		}
		_CameraXYZCoefficients(cameraXYZ);
	}

#if 0
	if (asn[0])
    	FORCC pre_multipliers[c] = 1 / asn[c];
#endif
	if (!useColorMatrix) {
		for (uint32 c = 0; c < fColors; c++) {
			fMeta.pre_multipliers[c] /= cameraCalibration[c][c];
		}
	}

	return B_OK;
}


status_t
DCRaw::_ParseTIFFImageFileDirectory(off_t baseOffset)
{
	while (fNumImages < kImageBufferCount) {
		int32 offset;
		fRead(offset);
		if (offset == 0)
			break;

		status_t status = _ParseTIFFImageFileDirectory(baseOffset, offset);
		if (status < B_OK)
			return status;

		fNumImages++;
	}

	return B_OK;
}


status_t
DCRaw::_ParseTIFF(off_t baseOffset)
{
	fRead.Stream().Seek(baseOffset, SEEK_SET);

	uint16 endian;
	fRead(endian);
	if (endian != 'MM' && endian != 'II')
		return B_NO_TRANSLATOR;

#if B_HOST_IS_LENDIAN
	fRead.SetSwap(endian == 'MM');
#else
	fRead.SetSwap(endian == 'II');
#endif

	fRead(endian);
		// dummy, not used, should be 42 for actual TIFF images,
		// but may vary for RAW images

	_ParseTIFFImageFileDirectory(baseOffset);
	fIsTIFF = true;

	uint32 maxSamples = 0;

	if (fThumbIndex >= 0 && _Thumb().data_offset) {
		fRead.Seek(_Thumb().data_offset, SEEK_SET);

		jhead jh;
		if (_LosslessJPEGInit(&jh, true)) {
			_Thumb().bits_per_sample = jh.bits;
			_Thumb().width = jh.wide;
			_Thumb().height = jh.high;
			_Thumb().bits_per_sample = 16;
		}
	}

	// identify RAW image in list of images retrieved

	for (uint32 i = 0; i < fNumImages; i++) {
		if (maxSamples < fImages[i].samples)
			maxSamples = fImages[i].samples;

		if ((fImages[i].compression != COMPRESSION_OLD_JPEG
				|| fImages[i].samples != 3)
			&& _SupportsCompression(fImages[i])) {
			fImages[i].is_raw = true;

			if (fRawIndex < 0 || fImages[i].width * fImages[i].height
					> _Raw().width * _Raw().height) {
				fRawIndex = i;
				//fuji_secondary = _Raw().samples == 2;
			}
		}
	}

	if (fRawIndex < 0
		|| (!fDNGVersion && _Raw().samples == 3 && _Raw().bits_per_sample == 8))
		throw (status_t)B_NO_TRANSLATOR;

	if (fRawIndex >= 0) {
		fMeta.raw_width = _Raw().width;
		fMeta.raw_height = _Raw().height;
	}

#if 0
  fuji_width *= (raw_width+1)/2;
  if (tiff_ifd[0].flip) tiff_flip = tiff_ifd[0].flip;
  if (raw >= 0 && !load_raw)
    switch (tiff_compress) {
      case 0:  case 1:
	load_raw = tiff_bps > 8 ?
	  &CLASS unpacked_load_raw : &CLASS eight_bit_load_raw;
	if (tiff_ifd[raw].bytes * 5 == raw_width * raw_height * 8)
	  load_raw = &CLASS olympus_e300_load_raw;
	if (tiff_bps == 12 && tiff_ifd[raw].phint == 2)
	  load_raw = &CLASS olympus_cseries_load_raw;
	break;
      case 6:  case 7:  case 99:
	load_raw = &CLASS lossless_jpeg_load_raw;		break;
      case 262:
	load_raw = &CLASS kodak_262_load_raw;			break;
      case 32773:
	load_raw = &CLASS packed_12_load_raw;			break;
      case 65535:
	load_raw = &CLASS pentax_k10_load_raw;			break;
      case 65000:
	switch (tiff_ifd[raw].phint) {
	  case 2: load_raw = &CLASS kodak_rgb_load_raw;   fFilters = 0;  break;
	  case 6: load_raw = &CLASS kodak_ycbcr_load_raw; fFilters = 0;  break;
	  case 32803: load_raw = &CLASS kodak_65000_load_raw;
	}
    }
  if (tiff_samples == 3 && tiff_bps == 8)
    if (!dng_version) is_raw = 0;
#endif

#if 0
  if (thm >= 0) {
    thumb_misc |= tiff_ifd[thm].samples << 5;
    switch (tiff_ifd[thm].comp) {
      case 0:
	write_thumb = &CLASS layer_thumb;
	break;
      case 1:
	if (tiff_ifd[thm].bps > 8)
	  thumb_load_raw = &CLASS kodak_thumb_load_raw;
	else
	  write_thumb = &CLASS ppm_thumb;
	break;
      case 65000:
	thumb_load_raw = tiff_ifd[thm].phint == 6 ?
		&CLASS kodak_ycbcr_load_raw : &CLASS kodak_rgb_load_raw;
    }
  }
#endif
	return B_OK;
}


//	#pragma mark -


status_t
DCRaw::Identify()
{
	fRead.Seek(0, SEEK_SET);

	status_t status = B_NO_TRANSLATOR;
	char header[32];
	fRead(header, sizeof(header));

	// check for TIFF-like files first

	uint16 endian = *(uint16*)&header;
	if (endian == 'II' || endian == 'MM')
		status = _ParseTIFF(0);

	if (status < B_OK)
		return status;

	// brush up some variables for later use

	fInputWidth = _Raw().width;
	fInputHeight = _Raw().height;

	_FixupValues();

	if ((_Raw().width | _Raw().height) < 0)
		_Raw().width = _Raw().height = 0;
	if (fMeta.maximum == 0)
		fMeta.maximum = (1 << _Raw().bits_per_sample) - 1;

	if (fFilters == ~0UL)
		fFilters = 0x94949494;
	if (fFilters && fColors == 3) {
		for (int32 i = 0; i < 32; i += 4) {
			if ((fFilters >> i & 15) == 9)
				fFilters |= 2 << i;
			if ((fFilters >> i & 15) == 6)
				fFilters |= 8 << i;
		}
	}

	if (fRawColor)
		_AdobeCoefficients(fMeta.manufacturer, fMeta.model);

	// remove invalid images

	int32 rawCount = 0;

	for (int32 i = 0; i < (int32)fNumImages; i++) {
		if (fImages[i].width == 0 || fImages[i].height == 0
			|| fImages[i].data_offset == 0) {
			fNumImages--;
			if (i == fRawIndex)
				fRawIndex = -1;
			else if (i < fRawIndex)
				fRawIndex--;
			if (i == fThumbIndex)
				fThumbIndex = -1;
			else if (i < fThumbIndex)
				fThumbIndex--;

			if (i < (int32)fNumImages) {
				memmove(&fImages[i], &fImages[i + 1],
					sizeof(image_data_info) * (fNumImages - i));
			}
			i--;
		} else if (fImages[i].is_raw)
			rawCount++;
	}

	// This is to prevent us from identifying TIFF images
	if (rawCount == 0)
		return B_NO_TRANSLATOR;

	fMeta.flip = _Raw().flip;
	return B_OK;
}


status_t
DCRaw::ReadImageAt(uint32 index, uint8*& outputBuffer, size_t& bufferSize)
{
	if (index >= fNumImages)
		return B_BAD_VALUE;

	_CorrectIndex(index);

	image_data_info& image = fImages[index];

	fShrink = (fHalfSize || fThreshold) && fFilters;
	fOutputWidth = (fInputWidth + fShrink) >> fShrink;
	fOutputHeight = (fInputHeight + fShrink) >> fShrink;

	if (image.flip > 4) {
		// image is rotated
		image.output_width = fOutputHeight;
		image.output_height = fOutputWidth;
	} else {
		image.output_width = fOutputWidth;
		image.output_height = fOutputHeight;
	}

	if (image.is_raw) {
		bufferSize = fOutputWidth * 4 * fOutputHeight;

		fImageData = (uint16 (*)[4])calloc(fOutputWidth * fOutputHeight
			* sizeof(*fImageData) + 0, 1); //meta_length, 1);
		if (fImageData == NULL)
			throw (status_t)B_NO_MEMORY;
	} else {
		bufferSize = image.bytes + sizeof(tiff_header) + 10;
			// TIFF header plus EXIF identifier
	}

	outputBuffer = (uint8*)malloc(bufferSize);
	if (outputBuffer == NULL) {
		free(fImageData);
		fImageData = NULL;
		throw (status_t)B_NO_MEMORY;
	}

	fRead.Seek(image.data_offset, SEEK_SET);

	if (image.is_raw) {
		_LoadRAW(image);

		//bad_pixels();
		//if (dark_frame) subtract (dark_frame);
		//quality = 2 + !fuji_width;

		if (fDocumentMode < 2)
			_ScaleColors();
		_PreInterpolate();
		_CameraToCIELab(NULL, NULL);

		if (fFilters && !fDocumentMode) {
#if 0
			if (quality == 0)
				lin_interpolate();
			else if (quality < 3 || colors > 3)
				vng_interpolate();
#endif
			_AHDInterpolate();
		}

#if 0
		if (fHightlight > 1)
			_RecoverHighlights();
		if (use_fuji_rotate) fuji_rotate();
		if (mix_green && (colors = 3))
			for (i=0; i < height*width; i++)
				image[i][1] = (image[i][1] + image[i][3]) >> 1;
#endif

		_ConvertToRGB();
		//if (use_fuji_rotate) stretch();

		_WriteRGB32(image, outputBuffer);
	} else {
		_WriteJPEG(image, outputBuffer);
	}

	free(fImageData);
	fImageData = NULL;

	return B_OK;
}


void
DCRaw::GetMetaInfo(image_meta_info& metaInfo) const
{
	metaInfo = fMeta;
}


uint32
DCRaw::CountImages() const
{
	return fNumImages;
}


status_t
DCRaw::ImageAt(uint32 index, image_data_info& info) const
{
	if (index >= fNumImages)
		return B_BAD_VALUE;

	_CorrectIndex(index);

	info = fImages[index];
	return B_OK;
}


status_t
DCRaw::GetEXIFTag(off_t& offset, size_t& length, bool& bigEndian) const
{
	if (fEXIFOffset < 0)
		return B_ENTRY_NOT_FOUND;

	offset = fEXIFOffset;
	length = fEXIFLength;

#if B_HOST_IS_LENDIAN
	bigEndian = fRead.IsSwapping();
#else
	bigEndian = !fRead.IsSwapping();
#endif
	return B_OK;
}


void
DCRaw::SetProgressMonitor(monitor_hook hook, void* data)
{
	fProgressMonitor = hook;
	fProgressData = data;
}


void
DCRaw::SetHalfSize(bool half)
{
	fHalfSize = half;
}
