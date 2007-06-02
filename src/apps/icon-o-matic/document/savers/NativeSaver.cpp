/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "NativeSaver.h"

#include "FlatIconFormat.h"
#include "MessageExporter.h"

// constructor
NativeSaver::NativeSaver(const entry_ref& ref)
	: fAttrSaver(ref, kVectorAttrNodeName),
	  fFileSaver(new MessageExporter(), ref)
{
}

// destructor
NativeSaver::~NativeSaver()
{
}

// Save
status_t
NativeSaver::Save(Document* document)
{
	status_t ret = fFileSaver.Save(document);
	if (ret < B_OK)
		return ret;
	return fAttrSaver.Save(document);
}

