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

#include "GIFTranslator.h"
#include "GIFWindow.h"
#include "GIFView.h"
#include "GIFSave.h"
#include "GIFLoad.h"


#include <ByteOrder.h>
#include <Catalog.h>
#include <DataIO.h>
#include <InterfaceDefs.h>
#include <TypeConstants.h>
#include <TranslatorAddOn.h>
#include <TranslatorFormats.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#ifndef GIF_TYPE
#define GIF_TYPE 'GIF '
#endif

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "GIFTranslator"


// This global will be externed in other files - set once here
// for the entire translator
bool debug = false;

bool DetermineType(BPositionIO *source, bool *is_gif);
status_t GetBitmap(BPositionIO *in, BBitmap **out);

/* Required data */
char translatorName[] = "GIF images";
char translatorInfo[] = "GIF image translator v1.4";
int32 translatorVersion = 0x140;

translation_format inputFormats[] = {
	{ GIF_TYPE, B_TRANSLATOR_BITMAP, 0.8, 0.8, "image/gif", 
		"GIF image" },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.3, 0.3, "image/x-be-bitmap", 
		"Be Bitmap Format (GIFTranslator)" },
	{ 0 }
};

translation_format outputFormats[] = {
	{ GIF_TYPE, B_TRANSLATOR_BITMAP, 0.8, 0.8, "image/gif", 
		"GIF image" },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.3, 0.3, "image/x-be-bitmap", 
		"Be Bitmap Format (GIFTranslator)" },
	{ 0 }
};


/* Build a pretty view for DataTranslations */
status_t
MakeConfig(BMessage *ioExtension, BView **outView, BRect *outExtent)
{
	GIFView *gifview = new GIFView("TranslatorView");
	*outView = gifview;
	gifview->ResizeTo(gifview->ExplicitPreferredSize());
	*outExtent = gifview->Bounds();
	return B_OK;
}


/* Look at first few bytes in stream to determine type - throw it back
   if it is not a GIF or a BBitmap that we understand */
bool
DetermineType(BPositionIO *source, bool *is_gif)
{
	unsigned char header[7];
	*is_gif = true;
	if (source->Read(header, 6) != 6) return false;
	header[6] = 0x00;

	if (strcmp((char *)header, "GIF87a") != 0 && strcmp((char *)header, 
		"GIF89a") != 0) {
		*is_gif = false;
		int32 magic = (header[0] << 24) + (header[1] << 16) + (header[2] << 8) 
			+ header[3];
		if (magic != B_TRANSLATOR_BITMAP) return false;
		source->Seek(5 * 4 - 2, SEEK_CUR);
		color_space cs;
		if (source->Read(&cs, 4) != 4) return false;
		cs = (color_space)B_BENDIAN_TO_HOST_INT32(cs);
		if (cs != B_RGB32 && cs != B_RGBA32 && cs != B_RGB32_BIG && cs 
			!= B_RGBA32_BIG) return false;
	}

	source->Seek(0, SEEK_SET);
	return true;
}


/* Dump data from stream into a BBitmap */
status_t
GetBitmap(BPositionIO *in, BBitmap **out)
{
	TranslatorBitmap header;

	status_t err = in->Read(&header, sizeof(header));
	if (err != sizeof(header))
		return B_IO_ERROR;

	header.magic = B_BENDIAN_TO_HOST_INT32(header.magic);
	header.bounds.left = B_BENDIAN_TO_HOST_FLOAT(header.bounds.left);
	header.bounds.top = B_BENDIAN_TO_HOST_FLOAT(header.bounds.top);
	header.bounds.right = B_BENDIAN_TO_HOST_FLOAT(header.bounds.right);
	header.bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(header.bounds.bottom);
	header.rowBytes = B_BENDIAN_TO_HOST_INT32(header.rowBytes);
	header.colors = (color_space)B_BENDIAN_TO_HOST_INT32(header.colors);
	header.dataSize = B_BENDIAN_TO_HOST_INT32(header.dataSize);

	BBitmap *bitmap = new BBitmap(header.bounds, header.colors);
	*out = bitmap;
	if (bitmap == NULL) return B_NO_MEMORY;
	unsigned char *bits = (unsigned char *)bitmap->Bits();
	if (bits == NULL) {
		delete bitmap;
		return B_NO_MEMORY;
	}
	err = in->Read(bits, header.dataSize);
	if (err == (status_t)header.dataSize) return B_OK;
	else {
		delete bitmap;
		return B_IO_ERROR;
	}
}


/* Required Identify function - may need to read entire header, not sure */
status_t
Identify(BPositionIO *inSource, const translation_format *inFormat,
	BMessage *ioExtension, translator_info *outInfo, uint32 outType)
{

	const char *debug_text = getenv("GIF_TRANSLATOR_DEBUG");
	if ((debug_text != NULL) && (atoi(debug_text) != 0)) debug = true;

	if (outType == 0) outType = B_TRANSLATOR_BITMAP;
	if (outType != GIF_TYPE && outType != B_TRANSLATOR_BITMAP) 
		return B_NO_TRANSLATOR;

	bool is_gif;
	if (!DetermineType(inSource, &is_gif)) return B_NO_TRANSLATOR;
	if (!is_gif && inFormat != NULL && inFormat->type != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	outInfo->group = B_TRANSLATOR_BITMAP;
	if (is_gif) {
		outInfo->type = GIF_TYPE;
		outInfo->quality = 0.8;
		outInfo->capability = 0.8;
		strncpy(outInfo->name, B_TRANSLATE("GIF image"), sizeof(outInfo->name));
		strcpy(outInfo->MIME, "image/gif");
	}
	else {
		outInfo->type = B_TRANSLATOR_BITMAP;
		outInfo->quality = 0.3;
		outInfo->capability = 0.3;
		strncpy(outInfo->name, B_TRANSLATE("Be Bitmap Format (GIFTranslator)"),
			sizeof(outInfo->name));
		strcpy(outInfo->MIME, "image/x-be-bitmap");
	}
	return B_OK;
}


/* Main required function - assumes that an incoming GIF must be translated
   to a BBitmap, and vice versa - this could be improved */
status_t
Translate(BPositionIO *inSource, const translator_info *inInfo,
	BMessage *ioExtension, uint32 outType, BPositionIO *outDestination)
{

	const char *debug_text = getenv("GIF_TRANSLATOR_DEBUG");
	if ((debug_text != NULL) && (atoi(debug_text) != 0)) debug = true;

	if (outType == 0) outType = B_TRANSLATOR_BITMAP;
	if (outType != GIF_TYPE && outType != B_TRANSLATOR_BITMAP) {
		return B_NO_TRANSLATOR;
	}

	bool is_gif;
	if (!DetermineType(inSource, &is_gif)) return B_NO_TRANSLATOR;
	if (!is_gif && inInfo->type != B_TRANSLATOR_BITMAP) return B_NO_TRANSLATOR;

	status_t err = B_OK;
	bigtime_t now = system_time();
	// Going from BBitmap to GIF
	if (!is_gif) {
		BBitmap *bitmap = NULL;
		err = GetBitmap(inSource, &bitmap);
		if (err != B_OK)
			return err;
		GIFSave *gs = new GIFSave(bitmap, outDestination);
		if (gs->fatalerror) {
			delete gs;
			delete bitmap;
			return B_NO_MEMORY;
		}
		delete gs;
		delete bitmap;
	} else { // GIF to BBitmap
		GIFLoad *gl = new GIFLoad(inSource, outDestination);
		if (gl->fatalerror) {
			delete gl;
			return B_NO_MEMORY;
		}
		delete gl;
	}

	if (debug) {
		now = system_time() - now;
		syslog(LOG_ERR, "Translate() - Translation took %Ld microseconds\n", now);
	}
	return B_OK;
}


GIFTranslator::GIFTranslator()
	: BApplication("application/x-vnd.Haiku-GIFTranslator")
{
	BRect rect(100, 100, 339, 339);
	gifwindow = new GIFWindow(rect, B_TRANSLATE("GIF Settings"));
	gifwindow->Show();
}


int
main()
{
	GIFTranslator myapp;
	myapp.Run();
	return 0;
}

