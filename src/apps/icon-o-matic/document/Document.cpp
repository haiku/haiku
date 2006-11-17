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
#include "DocumentSaver.h"
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

	  fNativeSaver(NULL),
	  fExportSaver(NULL)
{
}

// destructor
Document::~Document()
{
	delete fCommandStack;
	delete fSelection;
	fIcon->Release();
	delete fNativeSaver;
	delete fExportSaver;
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

// SetNativeSaver
void
Document::SetNativeSaver(::DocumentSaver* saver)
{
	delete fNativeSaver;
	fNativeSaver = saver;
}

// SetExportSaver
void
Document::SetExportSaver(::DocumentSaver* saver)
{
	delete fExportSaver;
	fExportSaver = saver;
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
Document::MakeEmpty(bool includingSavers)
{
	fCommandStack->Clear();
	fSelection->DeselectAll();
	fIcon->MakeEmpty();

	if (includingSavers) {
		delete fNativeSaver;
		fNativeSaver = NULL;
		delete fExportSaver;
		fExportSaver = NULL;
	}
}


