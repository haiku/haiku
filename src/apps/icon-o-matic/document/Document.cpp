/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Document.h"

#include <new>

#include <stdio.h>

#include <Entry.h>

#include "CommandStack.h"
#include "Icon.h"
#include "Selection.h"

using std::nothrow;

// constructor
Document::Document(const char* name)
	: RWLocker("document rw lock"),
	  fIcon(new (nothrow) ::Icon()),
	  fCommandStack(new (nothrow) ::CommandStack()),
	  fSelection(new (nothrow) ::Selection()),

	  fName(name),
	  fRef(NULL)
{
}

// destructor
Document::~Document()
{
	delete fCommandStack;
printf("~Document() - fCommandStack deleted\n");
	delete fSelection;
printf("~Document() - fSelection deleted\n");
	delete fIcon;
printf("~Document() - fIcon deleted\n");
	delete fRef;
printf("~Document() - fRef deleted\n");
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

