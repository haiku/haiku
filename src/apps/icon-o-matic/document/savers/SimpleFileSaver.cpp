/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "SimpleFileSaver.h"

#include "Exporter.h"

// constructor
SimpleFileSaver::SimpleFileSaver(Exporter* exporter,
								 const entry_ref& ref)
	: FileSaver(ref),
	  fExporter(exporter)
{
	fExporter->SetSelfDestroy(false);
}

// destructor
SimpleFileSaver::~SimpleFileSaver()
{
	delete fExporter;
}

// Save
status_t
SimpleFileSaver::Save(Document* document)
{
	return fExporter->Export(document, fRef);
}

