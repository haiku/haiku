/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "BitmapSetSaver.h"

#include <stdio.h>

#include <String.h>

#include "BitmapExporter.h"


// constructor
BitmapSetSaver::BitmapSetSaver(const entry_ref& ref)
	: FileSaver(ref)
{
}


// destructor
BitmapSetSaver::~BitmapSetSaver()
{
}


// Save
status_t
BitmapSetSaver::Save(Document* document)
{
	entry_ref actualRef(fRef);
	BString name;

	int sizes[] = { 64, 32, 16 };

	for (size_t i = 0; i < B_COUNT_OF(sizes); i++) {
		name.SetToFormat("%s_%d.png", fRef.name, sizes[i]);
		actualRef.set_name(name.String());
		Exporter* exporter = new BitmapExporter(sizes[i]);
		exporter->SetSelfDestroy(true);
		exporter->Export(document, actualRef);
	}

	return B_OK;
}
