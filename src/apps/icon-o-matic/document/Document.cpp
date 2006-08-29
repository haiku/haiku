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
	  fRef(NULL),
	  fExportRef(NULL)
{
}

// destructor
Document::~Document()
{
	delete fCommandStack;
	delete fSelection;
	fIcon->Release();
	delete fRef;
	delete fExportRef;
}

// SetName
void
Document::SetName(const char* name)
{
	if (fName == name)
		return;

	fName = name;
	Notify();
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

// SetExportRef
void
Document::SetExportRef(const entry_ref& ref)
{
	if (!fExportRef)
		fExportRef = new (nothrow) entry_ref(ref);
	else
		*fExportRef = ref;
}

// SetIcon
void
Document::SetIcon(::Icon* icon)
{
	if (fIcon == icon)
		return;

	fIcon->Release();

	fIcon = icon;

	// we don't acquire, since we own the icon
}

// MakeEmpty
void
Document::MakeEmpty()
{
	fCommandStack->Clear();
	fSelection->DeselectAll();
	fIcon->MakeEmpty();

	delete fRef;
	fRef = NULL;
	delete fExportRef;
	fExportRef = NULL;
}


