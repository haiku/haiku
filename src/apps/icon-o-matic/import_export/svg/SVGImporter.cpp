/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "SVGImporter.h"

#include <Alert.h>
#include <Catalog.h>
#include <Entry.h>
#include <File.h>
#include <Locale.h>
#include <Path.h>

#include "DocumentBuilder.h"
#include "SVGParser.h"


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

	// peek into file to see if this could be an SVG file at all
	BFile file(path.Path(), B_READ_ONLY);
	ret = file.InitCheck();
	if (ret < B_OK)
		return ret;

	ssize_t size = 5;
	char buffer[size + 1];
	if (file.Read(buffer, size) != size)
		return B_ERROR;

	// 0 terminate
	buffer[size] = 0;
	if (strcasecmp(buffer, "<?xml") != 0) {
		// we might be  stretching it a bit, but what the heck
		return B_ERROR;
	}

    try {
		agg::svg::DocumentBuilder builder;
		agg::svg::Parser parser(builder);
		parser.parse(path.Path());
		ret = builder.GetIcon(icon, this, ref->name);
	} catch(agg::svg::exception& e) {
		char error[1024];
		sprintf(error, B_TRANSLATE("Failed to open the file '%s' as "
					   "an SVG document.\n\n"
					   "Error: %s"), ref->name, e.msg());
		BAlert* alert = new BAlert(B_TRANSLATE("load error"),
								   error, B_TRANSLATE("OK"), NULL, NULL,
								   B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
		ret = B_ERROR;
	}

	return ret;
}
