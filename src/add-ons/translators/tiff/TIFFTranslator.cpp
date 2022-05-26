/*
 * Copyright 2003-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Wilber
 *		Stephan Aßmus <stippi@yellowbites.com> (write support)
 */


#include "TIFFTranslator.h"
#include "TIFFView.h"

#include "tiffio.h"

#include <Catalog.h>
#include <stdio.h>
#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TIFFTranslator"


/*!
	How this works:

	libtiff has a special version of TIFFOpen() that gets passed custom
	functions for reading writing etc. and a handle. This handle in our case
	is a BPositionIO object, which libtiff passes on to the functions for reading
	writing etc. So when operations are performed on the TIFF* handle that is
	returned by TIFFOpen(), libtiff uses the special reading writing etc
	functions so that all stream io happens on the BPositionIO object.
*/


// The input formats that this translator supports.
static const translation_format sInputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_IN_QUALITY,
		BBT_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (TIFFTranslator)"
	},
	{
		B_TIFF_FORMAT,
		B_TRANSLATOR_BITMAP,
		TIFF_IN_QUALITY,
		TIFF_IN_CAPABILITY,
		"image/tiff",
		"TIFF image"
	}
};

// The output formats that this translator supports.
static const translation_format sOutputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_OUT_QUALITY,
		BBT_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (TIFFTranslator)"
	},
	{
		B_TIFF_FORMAT,
		B_TRANSLATOR_BITMAP,
		TIFF_OUT_QUALITY,
		TIFF_OUT_CAPABILITY,
		"image/tiff",
		"TIFF image"
	}
};

// Default settings for the Translator
static const TranSetting sDefaultSettings[] = {
	{B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false},
	{B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false},
	{TIFF_SETTING_COMPRESSION, TRAN_SETTING_INT32, COMPRESSION_LZW}
		// Compression is LZW by default
};

const uint32 kNumInputFormats = sizeof(sInputFormats) / sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats) / sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings) / sizeof(TranSetting);


// ---------------------------------------------------------------
// make_nth_translator
//
// Creates a TIFFTranslator object to be used by BTranslatorRoster
//
// Preconditions:
//
// Parameters: n,		The translator to return. Since
//						TIFFTranslator only publishes one
//						translator, it only returns a
//						TIFFTranslator if n == 0
//
//             you, 	The image_id of the add-on that
//						contains code (not used).
//
//             flags,	Has no meaning yet, should be 0.
//
// Postconditions:
//
// Returns: NULL if n is not zero,
//          a new TIFFTranslator if n is zero
// ---------------------------------------------------------------
BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (!n)
		return new TIFFTranslator();
	else
		return NULL;
}


//// libtiff Callback functions!

BPositionIO *
tiff_get_pio(thandle_t stream)
{
	BPositionIO *pio = NULL;
	pio = static_cast<BPositionIO *>(stream);
	if (!pio)
		debugger("pio is NULL");

	return pio;
}

tsize_t
tiff_read_proc(thandle_t stream, tdata_t buf, tsize_t size)
{
	return tiff_get_pio(stream)->Read(buf, size);
}

tsize_t
tiff_write_proc(thandle_t stream, tdata_t buf, tsize_t size)
{
	return tiff_get_pio(stream)->Write(buf, size);
}

toff_t
tiff_seek_proc(thandle_t stream, toff_t off, int whence)
{
	return tiff_get_pio(stream)->Seek(off, whence);
}

int
tiff_close_proc(thandle_t stream)
{
	tiff_get_pio(stream)->Seek(0, SEEK_SET);
	return 0;
}

toff_t
tiff_size_proc(thandle_t stream)
{
	BPositionIO *pio = tiff_get_pio(stream);
	off_t cur, end;
	cur = pio->Position();
	end = pio->Seek(0, SEEK_END);
	pio->Seek(cur, SEEK_SET);

	return end;
}

int
tiff_map_file_proc(thandle_t stream, tdata_t *pbase, toff_t *psize)
{
	// BeOS doesn't support mmap() so just return 0
	return 0;
}

void
tiff_unmap_file_proc(thandle_t stream, tdata_t base, toff_t size)
{
	return;
}


status_t
identify_tiff_header(BPositionIO *inSource, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType, TIFF **poutTIFF = NULL)
{
	// get TIFF handle
	TIFF* tif = TIFFClientOpen("TIFFTranslator", "r", inSource,
		tiff_read_proc, tiff_write_proc, tiff_seek_proc, tiff_close_proc,
		tiff_size_proc, tiff_map_file_proc, tiff_unmap_file_proc);
	if (!tif)
		return B_NO_TRANSLATOR;

	// count number of documents
	int32 documentCount = 0, documentIndex = 1;
	do {
		documentCount++;
	} while (TIFFReadDirectory(tif));

	if (ioExtension) {
		// Check if a document index has been specified
		if (ioExtension->FindInt32(DOCUMENT_INDEX, &documentIndex) != B_OK)
			documentIndex = 1;

		if (documentIndex < 1 || documentIndex > documentCount) {
			// document index is invalid
			fputs(B_TRANSLATE("identify_tiff_header: invalid "
				"document index\n"), stderr);
			return B_NO_TRANSLATOR;
		}
	}

	// identify the document the user specified or the first document
	// if the user did not specify which document they wanted to identify
	if (!TIFFSetDirectory(tif, documentIndex - 1)) {
		fputs(B_TRANSLATE("identify_tiff_header: couldn't set "
			"directory\n"), stderr);
		return B_NO_TRANSLATOR;
	}

	if (ioExtension) {
		// add page count to ioExtension
		ioExtension->RemoveName(DOCUMENT_COUNT);
		ioExtension->AddInt32(DOCUMENT_COUNT, documentCount);
	}

	if (outInfo) {
		outInfo->type = B_TIFF_FORMAT;
		outInfo->group = B_TRANSLATOR_BITMAP;
		outInfo->quality = TIFF_IN_QUALITY;
		outInfo->capability = TIFF_IN_CAPABILITY;
		strcpy(outInfo->MIME, "image/tiff");
		strlcpy(outInfo->name, B_TRANSLATE("TIFF image"),
			sizeof(outInfo->name));
	}

	if (!poutTIFF) {
		// close TIFF if caller is not interested in TIFF handle
		TIFFClose(tif);
	} else {
		// leave TIFF open and return handle if caller needs it
		*poutTIFF = tif;
	}

	return B_OK;
}


// How this works:
// Following are a couple of functions,
//
// convert_buffer_*   to convert a buffer in place to the TIFF native format
//
// convert_buffers_*  to convert from one buffer to another to the TIFF
//                    native format, additionally compensating for padding bytes
//                    I don't know if libTIFF can be set up to respect padding bytes,
//                    otherwise this whole thing could be simplified a bit.
//
// Additionally, there are two functions convert_buffer() and convert_buffers() that take
// a color_space as one of the arguments and pick the correct worker functions from there.
// This way I don't write any code more than once, for easier debugging and maintainance.


// convert_buffer_bgra_rgba
inline void
convert_buffer_bgra_rgba(uint8* buffer, uint32 rows, uint32 width,
	uint32 bytesPerRow)
{
	for (uint32 y = 0; y < rows; y++) {
		uint8* handle = buffer;
		for (uint32 x = 0; x < width; x++) {
			uint8 temp = handle[0];
			handle[0] = handle[2];
			handle[2] = temp;
			handle += 4;
		}
		buffer += bytesPerRow;
	}
}

// convert_buffer_argb_rgba
inline void
convert_buffer_argb_rgba(uint8* buffer, uint32 rows, uint32 width,
	uint32 bytesPerRow)
{
	for (uint32 y = 0; y < rows; y++) {
		uint8* handle = buffer;
		for (uint32 x = 0; x < width; x++) {
			uint8 temp = handle[0];
			handle[0] = handle[1];
			handle[1] = handle[2];
			handle[2] = handle[3];
			handle[3] = temp;
			handle += 4;
		}
		buffer += bytesPerRow;
	}
}

// convert_buffers_bgra_rgba
inline void
convert_buffers_bgra_rgba(uint8* inBuffer, uint8* outBuffer, uint32 rows,
	uint32 width, uint32 bytesPerRow)
{
	for (uint32 y = 0; y < rows; y++) {
		uint8* inHandle = inBuffer;
		uint8* outHandle = outBuffer;
		for (uint32 x = 0; x < width; x++) {
			outHandle[0] = inHandle[2];
			outHandle[1] = inHandle[1];
			outHandle[2] = inHandle[0];
			outHandle[3] = inHandle[3];
			inHandle += 4;
			outHandle += 4;
		}
		inBuffer += bytesPerRow;
		outBuffer += width * 4;
	}
}

// convert_buffers_argb_rgba
inline void
convert_buffers_argb_rgba(uint8* inBuffer, uint8* outBuffer, uint32 rows,
	uint32 width, uint32 bytesPerRow)
{
	for (uint32 y = 0; y < rows; y++) {
		uint8* inHandle = inBuffer;
		uint8* outHandle = outBuffer;
		for (uint32 x = 0; x < width; x++) {
			outHandle[0] = inHandle[1];
			outHandle[1] = inHandle[2];
			outHandle[2] = inHandle[3];
			outHandle[3] = inHandle[0];
			inHandle += 4;
			outHandle += 4;
		}
		inBuffer += bytesPerRow;
		outBuffer += width * 4;
	}
}

// convert_buffers_bgrX_rgb
inline void
convert_buffers_bgrX_rgb(uint8* inBuffer, uint8* outBuffer, uint32 rows,
	uint32 width, uint32 bytesPerRow, uint32 samplesPerPixel)
{
	for (uint32 y = 0; y < rows; y++) {
		uint8* inHandle = inBuffer;
		uint8* outHandle = outBuffer;
		for (uint32 x = 0; x < width; x++) {
			// the usage of temp is just in case inBuffer == outBuffer
			// (see convert_buffer() for B_RGB24)
			uint8 temp = inHandle[0];
			outHandle[0] = inHandle[2];
			outHandle[1] = inHandle[1];
			outHandle[2] = temp;
			inHandle += samplesPerPixel;
			outHandle += 3;
		}
		inBuffer += bytesPerRow;
		outBuffer += width * 3;
	}
}

// convert_buffers_rgbX_rgb
inline void
convert_buffers_rgbX_rgb(uint8* inBuffer, uint8* outBuffer, uint32 rows,
	uint32 width, uint32 bytesPerRow, uint32 samplesPerPixel)
{
	for (uint32 y = 0; y < rows; y++) {
		uint8* inHandle = inBuffer;
		uint8* outHandle = outBuffer;
		for (uint32 x = 0; x < width; x++) {
			outHandle[0] = inHandle[0];
			outHandle[1] = inHandle[1];
			outHandle[2] = inHandle[2];
			inHandle += samplesPerPixel;
			outHandle += 3;
		}
		inBuffer += bytesPerRow;
		outBuffer += width * 3;
	}
}


// convert_buffers_cmap
inline void
convert_buffers_cmap(uint8* inBuffer, uint8* outBuffer, uint32 rows,
	uint32 width, uint32 bytesPerRow)
{
	// compensate for bytesPerRow != width (padding bytes)
	// this function will not be called if bytesPerRow == width, btw
	for (uint32 y = 0; y < rows; y++) {
		_TIFFmemcpy(outBuffer, inBuffer, width);
		inBuffer += bytesPerRow;
		outBuffer += width;
	}
}

// convert_buffer
inline void
convert_buffer(color_space format, uint8* buffer, uint32 rows, uint32 width,
	uint32 bytesPerRow)
{
	switch (format) {
		case B_RGBA32:
			convert_buffer_bgra_rgba(buffer, rows, width, bytesPerRow);
			break;
		case B_RGBA32_BIG:
			convert_buffer_argb_rgba(buffer, rows, width, bytesPerRow);
			break;
//		case B_RGB32:
//		case B_RGB32_BIG:
//			these two cannot be encountered, since inBufferSize != bytesPerStrip
//			(we're stripping the unused "alpha" channel 32->24 bits)
		case B_RGB24:
			convert_buffers_bgrX_rgb(buffer, buffer, rows, width, bytesPerRow, 3);
			break;
//		case B_RGB24_BIG:
			// buffer already has the correct format
			break;
//		case B_CMAP8:
//		case B_GRAY8:
			// buffer already has the correct format
			break;
		default:
			break;
	}
}

// convert_buffers
inline void
convert_buffers(color_space format, uint8* inBuffer, uint8* outBuffer,
	uint32 rows, uint32 width, uint32 bytesPerRow)
{
	switch (format) {
		case B_RGBA32:
			convert_buffers_bgra_rgba(inBuffer, outBuffer, rows, width, bytesPerRow);
			break;
		case B_RGBA32_BIG:
			convert_buffers_argb_rgba(inBuffer, outBuffer, rows, width, bytesPerRow);
			break;
		case B_RGB32:
			convert_buffers_bgrX_rgb(inBuffer, outBuffer, rows, width, bytesPerRow, 4);
			break;
		case B_RGB32_BIG:
			convert_buffers_rgbX_rgb(inBuffer, outBuffer, rows, width, bytesPerRow, 4);
			break;
		case B_RGB24:
			convert_buffers_bgrX_rgb(inBuffer, outBuffer, rows, width, bytesPerRow, 3);
			break;
		case B_RGB24_BIG:
			convert_buffers_rgbX_rgb(inBuffer, outBuffer, rows, width, bytesPerRow, 3);
			break;
		case B_CMAP8:
		case B_GRAY8:
			convert_buffers_cmap(inBuffer, outBuffer, rows, width, bytesPerRow);
			break;
		default:
			break;
	}
}

// Sets up any additional TIFF fields for the color spaces it supports,
// determines if it needs one or two buffers to carry out any conversions,
// uses the various convert routines above to do the actual conversion,
// writes complete strips of data plus one strip of remaining data.
//
// write_tif_stream
status_t
write_tif_stream(TIFF* tif, BPositionIO* inSource, color_space format,
				 uint32 width, uint32 height, uint32 bytesPerRow,
				 uint32 rowsPerStrip, uint32 dataSize)
{
	uint32 bytesPerStrip = 0;

	// set up the TIFF fields about what channels we write
	switch (format) {
		case B_RGBA32:
		case B_RGBA32_BIG:
			uint16 extraSamples[1];
			extraSamples[0] = EXTRASAMPLE_UNASSALPHA;
			TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, extraSamples);
			TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 4);
			// going to write rgb + alpha channels
			bytesPerStrip = width * 4 * rowsPerStrip;
			break;
		case B_RGB32:
		case B_RGB32_BIG:
		case B_RGB24:
		case B_RGB24_BIG:
			TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
			// going to write just the rgb channels
			bytesPerStrip = width * 3 * rowsPerStrip;
			break;
		case B_CMAP8:
		case B_GRAY8:
			bytesPerStrip = width * rowsPerStrip;
			break;
		default:
			return B_BAD_VALUE;
	}

	uint32 remaining = dataSize;
	status_t ret = B_OK;
	// Write the information to the stream
	uint32 inBufferSize = bytesPerRow * rowsPerStrip;
	// allocate intermediate input buffer
	uint8* inBuffer = (uint8*)_TIFFmalloc(inBufferSize);
	ssize_t read, written = B_ERROR;
	// bytesPerStrip is the size of the buffer that libtiff expects to write per strip
	// it might be different to the size of the input buffer,
	// if that one contains padding bytes at the end of each row
	if (inBufferSize != bytesPerStrip) {
		// allocate a second buffer
		// (two buffers are needed since padding bytes have to be compensated for)
		uint8* outBuffer = (uint8*)_TIFFmalloc(bytesPerStrip);
		if (inBuffer && outBuffer) {
//printf("using two buffers\n");
			read = inSource->Read(inBuffer, inBufferSize);
			uint32 stripIndex = 0;
			while (read == (ssize_t)inBufferSize) {
//printf("writing bytes: %ld (strip: %ld)\n", read, stripIndex);
				// convert the buffers (channel order) and compensate
				// for bytesPerRow != samplesPerRow (padding bytes)
				convert_buffers(format, inBuffer, outBuffer,
								rowsPerStrip, width, bytesPerRow);
				// let libtiff write the encoded strip to the BPositionIO
				written = TIFFWriteEncodedStrip(tif, stripIndex, outBuffer, bytesPerStrip);
				stripIndex++;
				if (written < B_OK)
					break;
				remaining -= inBufferSize;
				read = inSource->Read(inBuffer, min_c(inBufferSize, remaining));
			}
			// write the rest of the remaining rows
			if (read < (ssize_t)inBufferSize && read > 0) {
//printf("writing remaining bytes: %ld\n", read);
				// convert the buffers (channel order) and compensate
				// for bytesPerRow != samplesPerRow (padding bytes)
				convert_buffers(format, inBuffer, outBuffer,
								read / bytesPerRow, width, bytesPerRow);
				// let libtiff write the encoded strip to the BPositionIO
				written = TIFFWriteEncodedStrip(tif, stripIndex, outBuffer, read);
				remaining -= read;
			}
		} else
			ret = B_NO_MEMORY;
		// clean up output buffer
		if (outBuffer)
			_TIFFfree(outBuffer);
	} else {
//printf("using one buffer\n");
		// the input buffer is all we need, we convert it in place
		if (inBuffer) {
			read = inSource->Read(inBuffer, inBufferSize);
			uint32 stripIndex = 0;
			while (read == (ssize_t)inBufferSize) {
//printf("writing bytes: %ld (strip: %ld)\n", read, stripIndex);
				// convert the buffer (channel order)
				convert_buffer(format, inBuffer,
							   rowsPerStrip, width, bytesPerRow);
				// let libtiff write the encoded strip to the BPositionIO
				written = TIFFWriteEncodedStrip(tif, stripIndex, inBuffer, bytesPerStrip);
				stripIndex++;
				if (written < 0)
					break;
				remaining -= inBufferSize;
				read = inSource->Read(inBuffer, min_c(inBufferSize, remaining));
			}
			// write the rest of the remaining rows
			if (read < (ssize_t)inBufferSize && read > 0) {
//printf("writing remaining bytes: %ld (strip: %ld)\n", read, stripIndex);
				// convert the buffers (channel order) and compensate
				// for bytesPerRow != samplesPerRow (padding bytes)
				convert_buffer(format, inBuffer,
							   read / bytesPerRow, width, bytesPerRow);
				// let libtiff write the encoded strip to the BPositionIO
				written = TIFFWriteEncodedStrip(tif, stripIndex, inBuffer, read);
				remaining -= read;
			}
		} else
			ret = B_NO_MEMORY;
	}
	// clean up input buffer
	if (inBuffer)
		_TIFFfree(inBuffer);
	// see if there was an error reading or writing the streams
	if (remaining > 0)
		// "written" may contain a more specific error
		ret = written < 0 ? written : B_ERROR;
	else
		ret = B_OK;

	return ret;
}


//	#pragma mark -


TIFFTranslator::TIFFTranslator()
	: BaseTranslator(B_TRANSLATE("TIFF images"),
		B_TRANSLATE("TIFF image translator"),
		TIFF_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"TIFFTranslator_Settings",
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_BITMAP, B_TIFF_FORMAT)
{
	// TODO: for now!
	TIFFSetErrorHandler(NULL);
}


TIFFTranslator::~TIFFTranslator()
{
}


status_t
TIFFTranslator::DerivedIdentify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	return identify_tiff_header(inSource, ioExtension, outInfo, outType);
}


status_t
TIFFTranslator::translate_from_bits(BPositionIO *inSource, uint32 outType,
	BPositionIO *outDestination)
{
	TranslatorBitmap bitsHeader;

	uint32 compression = fSettings->SetGetInt32(TIFF_SETTING_COMPRESSION);

	status_t result;
	result = identify_bits_header(inSource, NULL, &bitsHeader);
	if (result != B_OK)
		return result;

	// Translate B_TRANSLATOR_BITMAP to B_TIFF_FORMAT
	if (outType == B_TIFF_FORMAT) {
		// Set up TIFF header

		// get TIFF handle
		TIFF* tif = TIFFClientOpen("TIFFTranslator", "w", outDestination,
			tiff_read_proc, tiff_write_proc, tiff_seek_proc, tiff_close_proc,
			tiff_size_proc, tiff_map_file_proc, tiff_unmap_file_proc);
		if (!tif)
			return B_NO_TRANSLATOR;

		// common fields which are independent of the bitmap format
		uint32 width = bitsHeader.bounds.IntegerWidth() + 1;
		uint32 height = bitsHeader.bounds.IntegerHeight() + 1;
		uint32 dataSize = bitsHeader.dataSize;
		uint32 bytesPerRow = bitsHeader.rowBytes;

		TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
		TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
		TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
/*const char* compressionString = NULL;
switch (compression) {
	case COMPRESSION_NONE:
		compressionString = "None";
		break;
	case COMPRESSION_PACKBITS:
		compressionString = "RLE";
		break;
	case COMPRESSION_DEFLATE:
		compressionString = "Deflate";
		break;
	case COMPRESSION_LZW:
		compressionString = "LZW";
		break;
	case COMPRESSION_JPEG:
		compressionString = "JPEG";
		break;
	case COMPRESSION_JP2000:
		compressionString = "JPEG2000";
		break;
}
if (compressionString)
printf("using compression: %s\n", compressionString);
else
printf("using unkown compression (%ld).\n", compression);
*/
		TIFFSetField(tif, TIFFTAG_COMPRESSION, compression);

		// TODO: some extra fields that should also get some special attention
		TIFFSetField(tif, TIFFTAG_XRESOLUTION, 150.0);
		TIFFSetField(tif, TIFFTAG_YRESOLUTION, 150.0);
		TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

		// we are going to write XX row(s) of pixels (lines) per strip
		uint32 rowsPerStrip = TIFFDefaultStripSize(tif, 0);
//printf("recommended rows per strip: %ld\n", TIFFDefaultStripSize(tif, 0));
		TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rowsPerStrip);

		status_t ret = B_OK;
		// set the rest of the fields according to the bitmap format
		switch (bitsHeader.colors) {

			// Output to 32-bit True Color TIFF (8 bits alpha)
			case B_RGBA32:
			case B_RGB32:
			case B_RGB24:
			case B_RGBA32_BIG:
			case B_RGB32_BIG:
			case B_RGB24_BIG:
				// set the fields specific to this color space
				TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
				TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
//				TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
				// write the tiff stream
				ret = write_tif_stream(tif, inSource, bitsHeader.colors,
									   width, height, bytesPerRow,
									   rowsPerStrip, dataSize);
				break;
/*
			case B_CMYA32:
				break;

			// Output to 15-bit True Color TIFF
			case B_RGB15:
			case B_RGB15_BIG:
				TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 5);
				TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
				TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
				bytesPerStrip = width * 2 * rowsPerStrip;
				break;
*/
			// Output to 8-bit Color Mapped TIFF 32 bits per color map entry
			case B_CMAP8: {
				TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
				TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
				TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
				// convert the system palette to 16 bit values for libtiff
				const color_map *map = system_colors();
				if (map) {
					uint16 red[256];
					uint16 green[256];
					uint16 blue[256];
					for (uint32 i = 0; i < 256; i++) {
						// scale 8 bits to 16 bits
						red[i] = map->color_list[i].red * 256 + map->color_list[i].red;
						green[i] = map->color_list[i].green * 256 + map->color_list[i].green;
						blue[i] = map->color_list[i].blue * 256 + map->color_list[i].blue;
					}
					TIFFSetField(tif, TIFFTAG_COLORMAP, &red, &green, &blue);
					// write the tiff stream
					ret = write_tif_stream(tif, inSource, bitsHeader.colors,
										   width, height, bytesPerRow,
										   rowsPerStrip, dataSize);
				} else
					ret = B_ERROR;
				break;
			}
			// Output to 8-bit Black and White TIFF
			case B_GRAY8:
				TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
				TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
				TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
				ret = write_tif_stream(tif, inSource, bitsHeader.colors,
									   width, height, bytesPerRow,
									   rowsPerStrip, dataSize);
				break;

/*			// Output to 1-bit Black and White TIFF
			case B_GRAY1:
				TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 1);
				TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 8);
				TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
				bytesPerStrip = ((width + 7) / 8) * rowsPerStrip;
				break;
*/
			default:
				ret = B_NO_TRANSLATOR;
		}
		// Close the handle
		TIFFClose(tif);
		return ret;

	} else
		return B_NO_TRANSLATOR;
}

status_t
TIFFTranslator::translate_from_tiff(BPositionIO *inSource, BMessage *ioExtension,
	uint32 outType, BPositionIO *outDestination)
{
	status_t result = B_NO_TRANSLATOR;

	bool bheaderonly = false, bdataonly = false;
		// Always write out the entire image. Some programs
		// fail when given "headerOnly", even though they requested it.
		// These settings are not applicable when outputting TIFFs

	// variables needing cleanup
	TIFF *ptif = NULL;
	uint32 *praster = NULL;

	status_t ret;
	ret = identify_tiff_header(inSource, ioExtension, NULL, outType, &ptif);

	if (outType == B_TIFF_FORMAT && ret == B_OK && ptif) {
		// if translating from TIFF to TIFF,
		// just write out the entire TIFF
		TIFFClose(ptif);
		translate_direct_copy(inSource, outDestination);
		return B_OK;
	}

	while (ret == B_OK && ptif) {
		// use while / break not for looping, but for
		// cleaner goto like capability

		ret = B_ERROR;
			// make certain there is no looping

		uint32 width = 0, height = 0;
		if (!TIFFGetField(ptif, TIFFTAG_IMAGEWIDTH, &width)) {
			result = B_NO_TRANSLATOR;
			break;
		}
		if (!TIFFGetField(ptif, TIFFTAG_IMAGELENGTH, &height)) {
			result = B_NO_TRANSLATOR;
			break;
		}
		size_t npixels = 0;
		npixels = width * height;
		praster = static_cast<uint32 *>(_TIFFmalloc(npixels * 4));
		if (praster && TIFFReadRGBAImage(ptif, width, height, praster, 0)) {
			if (!bdataonly) {
				// Construct and write Be bitmap header
				TranslatorBitmap bitsHeader;
				bitsHeader.magic = B_TRANSLATOR_BITMAP;
				bitsHeader.bounds.left = 0;
				bitsHeader.bounds.top = 0;
				bitsHeader.bounds.right = width - 1;
				bitsHeader.bounds.bottom = height - 1;
				bitsHeader.rowBytes = 4 * width;
				bitsHeader.colors = B_RGBA32;
				bitsHeader.dataSize = bitsHeader.rowBytes * height;
				if (swap_data(B_UINT32_TYPE, &bitsHeader,
					sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN) != B_OK) {
					result = B_ERROR;
					break;
				}
				outDestination->Write(&bitsHeader, sizeof(TranslatorBitmap));
			}

			if (!bheaderonly) {
				// Convert raw RGBA data to B_RGBA32 colorspace
				// and write out the results
				uint8 *pbitsrow = new uint8[width * 4];
				if (!pbitsrow) {
					result = B_NO_MEMORY;
					break;
				}
				uint8 *pras8 = reinterpret_cast<uint8 *>(praster);
				for (uint32 i = 0; i < height; i++) {
					uint8 *pbits, *prgba;
					pbits = pbitsrow;
					prgba = pras8 + ((height - (i + 1)) * width * 4);

					for (uint32 k = 0; k < width; k++) {
						pbits[0] = prgba[2];
						pbits[1] = prgba[1];
						pbits[2] = prgba[0];
						pbits[3] = prgba[3];
						pbits += 4;
						prgba += 4;
					}

					outDestination->Write(pbitsrow, width * 4);
				}
				delete[] pbitsrow;
				pbitsrow = NULL;
			}

			result = B_OK;
			break;

		} // if (praster && TIFFReadRGBAImage(ptif, width, height, praster, 0))

	} // while (ret == B_OK && ptif)

	if (praster) {
		_TIFFfree(praster);
		praster = NULL;
	}
	if (ptif) {
		TIFFClose(ptif);
		ptif = NULL;
	}

	return result;
}

// ---------------------------------------------------------------
// DerivedTranslate
//
// Translates the data in inSource to the type outType and stores
// the translated data in outDestination.
//
// Preconditions:
//
// Parameters:	inSource,	the data to be translated
//
//				inInfo,	hint about the data in inSource (not used)
//
//				ioExtension,	configuration options for the
//								translator
//
//				outType,	the type to convert inSource to
//
//				outDestination,	where the translated data is
//								put
//
//				baseType, indicates whether inSource is in the
//				          bits format, not in the bits format or
//				          is unknown
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if the options in ioExtension are bad
//
// B_NO_TRANSLATOR, if this translator doesn't understand the data
//
// B_ERROR, if there was an error allocating memory or converting
//          data
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
TIFFTranslator::DerivedTranslate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination, int32 baseType)
{
	if (baseType == 1)
		// if inSource is in bits format
		return translate_from_bits(inSource, outType, outDestination);
	else if (baseType == 0)
		// if inSource is NOT in bits format
		return translate_from_tiff(inSource, ioExtension, outType, outDestination);
	else
		// if BaseTranslator did not properly identify the data as
		// bits or not bits
		return B_NO_TRANSLATOR;
}

BView *
TIFFTranslator::NewConfigView(TranslatorSettings *settings)
{
	return new TIFFView(B_TRANSLATE("TIFFTranslator Settings"),
		B_WILL_DRAW, settings);
}
