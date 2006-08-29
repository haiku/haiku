/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Importer.h"

#include "Icon.h"
#include "PathContainer.h"
#include "StyleContainer.h"

// constructor
Importer::Importer()
	: fStyleIndexOffset(0),
	  fPathIndexOffset(0)
{
}

// destructor
Importer::~Importer()
{
}

// Export
status_t
Importer::Init(Icon* icon)
{
	fStyleIndexOffset = 0;
	fPathIndexOffset = 0;

	if (!icon || icon->InitCheck() < B_OK)
		return B_BAD_VALUE;

	fStyleIndexOffset = icon->Styles()->CountStyles();
	fPathIndexOffset = icon->Paths()->CountPaths();

	return B_OK;
}

// StyleIndexFor
int32
Importer::StyleIndexFor(int32 savedIndex) const
{
	return savedIndex + fStyleIndexOffset;
}

// PathIndexFor
int32
Importer::PathIndexFor(int32 savedIndex) const
{
	return savedIndex + fPathIndexOffset;
}


