/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "BitmapSetSaver.h"

#include <stdio.h>

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
	char name[B_OS_NAME_LENGTH];

	// 64x64
	sprintf(name, "%s_64.png", fRef.name);
	actualRef.set_name(name);
	Exporter* exporter = new BitmapExporter(64);
	exporter->SetSelfDestroy(true);
	exporter->Export(document, actualRef);

	// 16x16
	sprintf(name, "%s_16.png", fRef.name);
	actualRef.set_name(name);
	exporter = new BitmapExporter(16);
	exporter->SetSelfDestroy(true);
	exporter->Export(document, actualRef);

	// 32x32
	sprintf(name, "%s_32.png", fRef.name);
	actualRef.set_name(name);
	exporter = new BitmapExporter(32);
	exporter->SetSelfDestroy(true);
	exporter->Export(document, actualRef);

	return B_OK;
}

