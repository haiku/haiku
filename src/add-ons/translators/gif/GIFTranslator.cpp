////////////////////////////////////////////////////////////////////////////////
//
//	File: GIFTranslator.cpp
//
//	Date: December 1999
//
//	Author: Daniel Switkin
//
//	Copyright 2003 (c) by Daniel Switkin. This file is made publically available
//	under the BSD license, with the stipulations that this complete header must
//	remain at the top of the file indefinitely, and credit must be given to the
//	original author in any about box using this software.
//
////////////////////////////////////////////////////////////////////////////////

// Additional authors:	Stephan Aßmus, <superstippi@gmx.de>
//						John Scipione, <jscipione@gmail.com>

#include "GIFTranslator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <Application.h>
#include <ByteOrder.h>
#include <Catalog.h>
#include <DataIO.h>
#include <InterfaceDefs.h>
#include <TranslatorAddOn.h>
#include <TranslatorFormats.h>
#include <TypeConstants.h>

#include "GIFLoad.h"
#include "GIFSave.h"
#include "GIFView.h"
#include "TranslatorWindow.h"


#ifndef GIF_TYPE
#define GIF_TYPE 'GIF '
#endif

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GIFTranslator"


bool debug = false;
	// this global will be externed in other files - set once here
	// for the entire translator

bool DetermineType(BPositionIO* source, bool* isGif);
status_t GetBitmap(BPositionIO* in, BBitmap** out);

static const translation_format sInputFormats[] = {
	{
		GIF_TYPE,
		B_TRANSLATOR_BITMAP,
		GIF_IN_QUALITY,
		GIF_IN_CAPABILITY,
		"image/gif",
		"GIF image"
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBM_IN_QUALITY,
		BBM_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (GIFTranslator)"
	}
};

static const translation_format sOutputFormats[] = {
	{
		GIF_TYPE,
		B_TRANSLATOR_BITMAP,
		GIF_OUT_QUALITY,
		GIF_OUT_CAPABILITY,
		"image/gif",
		"GIF image"
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBM_OUT_QUALITY,
		BBM_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (GIFTranslator)"
	}
};

// Default settings for the Translator
static const TranSetting sDefaultSettings[] = {
	{ B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false },
	{ B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false },
	{ GIF_SETTING_INTERLACED, TRAN_SETTING_BOOL, false },
	{ GIF_SETTING_USE_TRANSPARENT, TRAN_SETTING_BOOL, true },
	{ GIF_SETTING_USE_TRANSPARENT_AUTO, TRAN_SETTING_BOOL, true },
	{ GIF_SETTING_USE_DITHERING, TRAN_SETTING_BOOL, false },
	{ GIF_SETTING_PALETTE_MODE, TRAN_SETTING_INT32, 3 },
	{ GIF_SETTING_PALETTE_SIZE, TRAN_SETTING_INT32, 8 },
	{ GIF_SETTING_TRANSPARENT_RED, TRAN_SETTING_INT32, 255 },
	{ GIF_SETTING_TRANSPARENT_GREEN, TRAN_SETTING_INT32, 255 },
	{ GIF_SETTING_TRANSPARENT_BLUE, TRAN_SETTING_INT32, 255 }
};

const uint32 kNumInputFormats = sizeof(sInputFormats)
	/ sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats)
	/ sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings)
	/ sizeof(TranSetting);


/*!	Look at first few bytes in stream to determine type - throw it back
	if it is not a GIF or a BBitmap that we understand.
*/
bool
DetermineType(BPositionIO* source, bool* isGif)
{
	unsigned char header[7];
	*isGif = true;
	if (source->Read(header, 6) != 6)
		return false;

	header[6] = 0x00;

	if (strcmp((char*)header, "GIF87a") != 0
		&& strcmp((char*)header, "GIF89a") != 0) {
		*isGif = false;
		int32 magic = (header[0] << 24) + (header[1] << 16) + (header[2] << 8)
			+ header[3];
		if (magic != B_TRANSLATOR_BITMAP)
			return false;

		source->Seek(5 * 4 - 2, SEEK_CUR);
		color_space cs;
		if (source->Read(&cs, 4) != 4)
			return false;

		cs = (color_space)B_BENDIAN_TO_HOST_INT32(cs);
		if (cs != B_RGB32 && cs != B_RGBA32 && cs != B_RGB32_BIG
			&& cs != B_RGBA32_BIG) {
			return false;
		}
	}

	source->Seek(0, SEEK_SET);
	return true;
}


status_t
GetBitmap(BPositionIO* in, BBitmap** out)
{
	TranslatorBitmap header;
	status_t result = in->Read(&header, sizeof(header));
	if (result != sizeof(header))
		return B_IO_ERROR;

	header.magic = B_BENDIAN_TO_HOST_INT32(header.magic);
	header.bounds.left = B_BENDIAN_TO_HOST_FLOAT(header.bounds.left);
	header.bounds.top = B_BENDIAN_TO_HOST_FLOAT(header.bounds.top);
	header.bounds.right = B_BENDIAN_TO_HOST_FLOAT(header.bounds.right);
	header.bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(header.bounds.bottom);
	header.rowBytes = B_BENDIAN_TO_HOST_INT32(header.rowBytes);
	header.colors = (color_space)B_BENDIAN_TO_HOST_INT32(header.colors);
	header.dataSize = B_BENDIAN_TO_HOST_INT32(header.dataSize);

	// dump data from stream into a BBitmap
	*out = new(std::nothrow) BBitmap(header.bounds, header.colors);
	if (*out == NULL)
		return B_NO_MEMORY;

	if (!(*out)->IsValid()) {
		delete *out;
		return B_NO_MEMORY;
	}

	result = in->Read((*out)->Bits(), header.dataSize);
	if (result != (status_t)header.dataSize) {
		delete *out;
		return B_IO_ERROR;
	}

	return B_OK;
}


/*!	Required identify function - may need to read entire header, not sure
*/
status_t
GIFTranslator::DerivedIdentify(BPositionIO* inSource,
	const translation_format* inFormat, BMessage* ioExtension,
	translator_info* outInfo, uint32 outType)
{
	const char* debug_text = getenv("GIF_TRANSLATOR_DEBUG");
	if (debug_text != NULL && atoi(debug_text) != 0)
		debug = true;

	if (outType == 0)
		outType = B_TRANSLATOR_BITMAP;

	if (outType != GIF_TYPE && outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	bool isGif;
	if (!DetermineType(inSource, &isGif))
		return B_NO_TRANSLATOR;

	if (!isGif && inFormat != NULL && inFormat->type != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	outInfo->group = B_TRANSLATOR_BITMAP;
	if (isGif) {
		outInfo->type = GIF_TYPE;
		outInfo->quality = GIF_IN_QUALITY;
		outInfo->capability = GIF_IN_CAPABILITY;
		strlcpy(outInfo->name, B_TRANSLATE("GIF image"), sizeof(outInfo->name));
		strcpy(outInfo->MIME, "image/gif");
	} else {
		outInfo->type = B_TRANSLATOR_BITMAP;
		outInfo->quality = BBM_IN_QUALITY;
		outInfo->capability = BBM_IN_CAPABILITY;
		strlcpy(outInfo->name, B_TRANSLATE("Be Bitmap Format (GIFTranslator)"),
			sizeof(outInfo->name));
		strcpy(outInfo->MIME, "image/x-be-bitmap");
	}

	return B_OK;
}


/*!	Main required function - assumes that an incoming GIF must be translated
	to a BBitmap, and vice versa - this could be improved
*/
status_t
GIFTranslator::DerivedTranslate(BPositionIO* inSource,
	const translator_info* inInfo, BMessage* ioExtension, uint32 outType,
	BPositionIO* outDestination, int32 baseType)
{
	const char* debug_text = getenv("GIF_TRANSLATOR_DEBUG");
	if (debug_text != NULL && atoi(debug_text) != 0)
		debug = true;

	if (outType == 0)
		outType = B_TRANSLATOR_BITMAP;

	if (outType != GIF_TYPE && outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	bool isGif;
	if (!DetermineType(inSource, &isGif))
		return B_NO_TRANSLATOR;

	if (!isGif && inInfo->type != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	status_t result = B_OK;
	bigtime_t now = system_time();

	if (!isGif) {
		// BBitmap to GIF
		BBitmap* bitmap;
		result = GetBitmap(inSource, &bitmap);
		if (result != B_OK)
			return result;

		GIFSave* gifSave = new(std::nothrow) GIFSave(bitmap, outDestination,
			fSettings);
		if (gifSave == NULL) {
			delete bitmap;
			return B_NO_MEMORY;
		}

		if (gifSave->fatalerror) {
			delete gifSave;
			delete bitmap;
			return B_NO_MEMORY;
		}
		delete gifSave;
		delete bitmap;
	} else {
		// GIF to BBitmap
		GIFLoad* gifLoad = new(std::nothrow) GIFLoad(inSource, outDestination);
		if (gifLoad == NULL)
			return B_NO_MEMORY;

		if (gifLoad->fatalerror) {
			delete gifLoad;
			return B_NO_MEMORY;
		}
		delete gifLoad;
	}

	if (debug) {
		now = system_time() - now;
		syslog(LOG_INFO, "Translate() - Translation took %Ld microseconds\n",
			now);
	}
	return B_OK;
}


BTranslator*
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (n == 0)
		return new(std::nothrow) GIFTranslator();

	return NULL;
}


GIFTranslator::GIFTranslator()
	:
	BaseTranslator(B_TRANSLATE("GIF images"),
	B_TRANSLATE("GIF image translator"),
	GIF_TRANSLATOR_VERSION,
	sInputFormats, kNumInputFormats,
	sOutputFormats, kNumOutputFormats,
	"GIFTranslator_Settings",
	sDefaultSettings, kNumDefaultSettings,
	B_TRANSLATOR_BITMAP, B_GIF_FORMAT)
{
}


GIFTranslator::~GIFTranslator()
{
}


BView*
GIFTranslator::NewConfigView(TranslatorSettings* settings)
{
	return new(std::nothrow) GIFView(settings);
}


int
main()
{
	BApplication app("application/x-vnd.Haiku-GIFTranslator");
	status_t result = LaunchTranslatorWindow(new(std::nothrow) GIFTranslator,
		B_TRANSLATE("GIF Settings"), kRectView);
	if (result == B_OK) {
		app.Run();
		return 0;
	}

	return 1;
}
