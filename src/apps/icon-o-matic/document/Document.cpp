/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Document.h"

#include <new>

#include <Entry.h>

#include "CommandStack.h"
#include "Selection.h"

using std::nothrow;

// constructor
Document::Document(const char* name)
	: RWLocker("document rw lock"),
	  fCommandStack(new (nothrow) ::CommandStack()),
	  fSelection(new (nothrow) ::Selection()),

	  fRef(NULL)
{
	SetName(name);
}

// destructor
Document::~Document()
{
	delete fCommandStack;
	delete fSelection;
	delete fRef;
}

// SetName
void
Document::SetName(const char* name)
{
	fName = name;
}

// Name
const char*
Document::Name() const
{
	return fName.String();
}

// SetRef
void
Document::SetRef(const entry_ref& ref)
{
	if (!fRef)
		fRef = new (nothrow) entry_ref(ref);
	else
		*fRef = ref;
}

