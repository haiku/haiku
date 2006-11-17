/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "FileSaver.h"

// constructor
FileSaver::FileSaver(const entry_ref& ref)
	: fRef(ref)
{
}

// destructor
FileSaver::~FileSaver()
{
}

// SetRef
void
FileSaver::SetRef(const entry_ref& ref)
{
	fRef = ref;
}

