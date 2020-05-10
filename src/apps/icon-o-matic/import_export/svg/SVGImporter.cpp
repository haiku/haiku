/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "SVGImporter.h"

#include <strings.h>

#include <Alert.h>
#include <Catalog.h>
#include <Entry.h>
#include <File.h>
#include <Locale.h>
#include <MimeType.h>
#include <Path.h>

#include "DocumentBuilder.h"
#include "nanosvg.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-SVGImport"


// constructor
SVGImporter::SVGImporter()
{
}

// destructor
SVGImporter::~SVGImporter()
{
}

// Import
status_t
SVGImporter::Import(Icon* icon, const entry_ref* ref)
{
	status_t ret = Init(icon);
	if (ret < B_OK) {
		printf("SVGImporter::Import() - "
			   "Init() error: %s\n", strerror(ret));
		return ret;
	}

	BPath path(ref);
	ret = path.InitCheck();
	if (ret < B_OK)
		return ret;

	// Check that it indeed looks like an SVG file
	BMimeType type;
	ret = BMimeType::GuessMimeType(ref, &type);
	if (ret != B_OK || strcmp(type.Type(), "image/svg+xml") != 0) {
		printf("not an svg file %s %s\n", strerror(ret), type.Type());
		return B_ERROR;
	}

	NSVGimage* svg = nsvgParseFromFile(path.Path(), "px", 96);
	if (svg == NULL) {
		char error[1024];
		sprintf(error, B_TRANSLATE("Failed to open the file '%s' as "
			"an SVG document.\n\n"), ref->name);
		BAlert* alert = new BAlert(B_TRANSLATE("load error"),
			error, B_TRANSLATE("OK"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
		return B_ERROR;
	}

	DocumentBuilder builder(svg);
	ret = builder.GetIcon(icon, this, ref->name);
	nsvgDelete(svg);

	return ret;
}
