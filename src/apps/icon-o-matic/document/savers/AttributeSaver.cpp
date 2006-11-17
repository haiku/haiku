/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "AttributeSaver.h"

#include <Node.h>

#include "Document.h"
#include "FlatIconExporter.h"

// constructor
AttributeSaver::AttributeSaver(const entry_ref& ref, const char* attrName)
	: fRef(ref),
	  fAttrName(attrName)
{
}

// destructor
AttributeSaver::~AttributeSaver()
{
}

// Save
status_t
AttributeSaver::Save(Document* document)
{
	BNode node(&fRef);
	FlatIconExporter iconExporter;
	return iconExporter.Export(document->Icon(), &node, fAttrName.String());
}

