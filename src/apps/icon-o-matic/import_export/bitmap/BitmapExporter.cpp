/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "BitmapExporter.h"

#include <Bitmap.h>
#include <BitmapStream.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>

#include "Icon.h"
#include "IconRenderer.h"

// constructor
BitmapExporter::BitmapExporter(uint32 size)
	: Exporter(),
	  fFormat(B_PNG_FORMAT),
	  fSize(size)
{
}

// destructor
BitmapExporter::~BitmapExporter()
{
}

// Export
status_t
BitmapExporter::Export(const Icon* icon, BPositionIO* stream)
{
	if (fSize == 0)
		return B_NO_INIT;

	// render icon into bitmap with given size and transparent background
	uint32 bitmapFlags = 0;

	#if __HAIKU__
	bitmapFlags |= B_BITMAP_NO_SERVER_LINK;
	#endif

	BBitmap bitmap(BRect(0, 0, fSize - 1, fSize - 1),
				   bitmapFlags, B_RGBA32);

	status_t ret  = bitmap.InitCheck();
	if (ret < B_OK)
		return ret;

	IconRenderer renderer(&bitmap);
	renderer.SetIcon(icon);
	renderer.SetScale(fSize / 64.0);
	renderer.Render();
//	renderer.Demultiply(&bitmap);

	// save bitmap to translator
	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (!roster)
		return B_ERROR;

	BBitmapStream bitmapStream(&bitmap);
	ret = roster->Translate(&bitmapStream, NULL, NULL, stream, fFormat, 0);

	BBitmap* dummy;
	bitmapStream.DetachBitmap(&dummy);

	return ret;
}

// MIMEType
const char*
BitmapExporter::MIMEType()
{
	// TODO: ...
	return "image/png";
}

