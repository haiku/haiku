/*
 * Copyright 2014, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "MidiView.h"

#include <Box.h>
#include <Button.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <GridView.h>
#include <LayoutBuilder.h>
#include <ListView.h>

#include <stdio.h>

const static uint32 kImportSoundFont = 'ImSf';

MidiView::MidiView()
	:
	BGroupView(B_VERTICAL, B_USE_DEFAULT_SPACING)
{
	BBox* defaultsBox = new BBox("defaults");
	defaultsBox->SetLabel("Defaults");
	BGridView* defaultsGridView = new BGridView();

	fListView = new BListView(B_SINGLE_SELECTION_LIST);
	fImportButton = new BButton("Import SoundFont",
			new BMessage(kImportSoundFont));

	BLayoutBuilder::Grid<>(defaultsGridView)
			.SetInsets(B_USE_DEFAULT_SPACING, 0, B_USE_DEFAULT_SPACING,
					B_USE_DEFAULT_SPACING)
			.Add(fListView, 0, 0)
			.Add(fImportButton, 0, 1);

	defaultsBox->AddChild(defaultsGridView);

	BLayoutBuilder::Group<>(this)
		.SetInsets(0, 0, 0, 0)
		.Add(defaultsBox)
		.AddGlue();
}


/* virtual */
void
MidiView::AttachedToWindow()
{
	fImportButton->SetTarget(this);
	_RetrieveSoftSynthList();
}


/* virtual */
void
MidiView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kImportSoundFont:
			break;

		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
MidiView::_RetrieveSoftSynthList()
{
	char **paths = NULL;
	size_t pathCount = 0;
	status_t status = find_paths(B_FIND_PATH_DATA_DIRECTORY, "synth",
			&paths, &pathCount);
	if (status != B_OK)
		return;

	fListView->MakeEmpty();

	for (size_t i = 0; i < pathCount; i++) {
		BDirectory directory(paths[i]);
		BEntry entry;
		if (directory.InitCheck() != B_OK)
			continue;
		while (directory.GetNextEntry(&entry) == B_OK) {
			fListView->AddItem(new BStringItem(entry.Name()));
		}
	}
}
