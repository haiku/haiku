/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GenericSettingsView.h"

#include "SettingsFile.h"

#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <String.h>
#include <TextControl.h>

#include <stdlib.h>


GenericSettingsView::GenericSettingsView(BRect frame, const char* name,
	SettingsFile* settings, uint32 resizingMode, uint32 flags)
	:
	BView(frame, name, resizingMode, flags)
{
	fSettings = settings;

	SetLayout(new BGroupLayout(B_VERTICAL));

	fUndoLimitText = new BTextControl(BRect(0, 0, 0, 0), "fUndoLimitText",
		"Undo Limit:", NULL, NULL);
	fUndoLimitText->SetDivider(100);

	// TODO: Add more controls for generic settings (2/4).

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 8)
		.Add(fUndoLimitText)
		.AddGlue()
	);
}


GenericSettingsView::~GenericSettingsView()
{

}


void
GenericSettingsView::ApplySettings()
{
	fSettings->UndoLimit = atoi(fUndoLimitText->Text());
	// TODO: Add more controls for generic settings (3/4).
}


void
GenericSettingsView::AdaptSettings()
{
	fUndoLimitText->SetText(BString() << fSettings->UndoLimit);
	// TODO: Add more controls for generic settings (4/4).
}
