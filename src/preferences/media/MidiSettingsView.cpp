/*
 * Copyright 2014, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "MidiSettingsView.h"

#include <Box.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <GridView.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <ScrollView.h>

#include <stdio.h>
#include <stdlib.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Midi View"

#define SETTINGS_FILE "midi"

const static uint32 kSelectSoundFont = 'SeSf';


MidiSettingsView::MidiSettingsView()
	:
	SettingsView()
{
	BBox* defaultsBox = new BBox("SoundFont");
	defaultsBox->SetLabel(B_TRANSLATE("SoundFont"));
	BGridView* defaultsGridView = new BGridView();

	fListView = new BListView(B_SINGLE_SELECTION_LIST);
	fListView->SetSelectionMessage(new BMessage(kSelectSoundFont));

	BScrollView *scrollView = new BScrollView("ScrollView", fListView,
			0, false, true);
	BLayoutBuilder::Grid<>(defaultsGridView)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING)
		.Add(scrollView, 0, 0);

	defaultsBox->AddChild(defaultsGridView);

	BLayoutBuilder::Group<>(this)
		.SetInsets(0, 0, 0, 0)
		.Add(defaultsBox)
		.AddGlue();
}


/* virtual */
void
MidiSettingsView::AttachedToWindow()
{
	SettingsView::AttachedToWindow();

	fListView->SetTarget(this);
	_RetrieveSoftSynthList();

	_LoadSettings();
}


/* virtual */
void
MidiSettingsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kSelectSoundFont:
			_SaveSettings();
			break;

		default:
			SettingsView::MessageReceived(message);
			break;
	}
}


void
MidiSettingsView::_RetrieveSoftSynthList()
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
			// TODO: Get rid of this as soon as we update
			// the SoundFont package not to create the link
			// anymore
			if (!strcmp(entry.Name(), "big_synth.sy"))
				continue;
			BNode node(&entry);
			BNodeInfo nodeInfo(&node);
			char mimeType[B_MIME_TYPE_LENGTH];
			// TODO: For some reason this doesn't work
			if (nodeInfo.GetType(mimeType) == B_OK
					/*&& !strcmp(mimeType, "audio/x-soundfont")*/) {
				BPath fullPath = paths[i];
				fullPath.Append(entry.Name());
				fListView->AddItem(new BStringItem(fullPath.Path()));
			}
		}
	}

	free(paths);
}


void
MidiSettingsView::_LoadSettings()
{
	// TODO: Duplicated code between here
	// and BSoftSynth::LoadDefaultInstruments

	char buffer[512];
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(SETTINGS_FILE);
		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK)
			file.Read(buffer, sizeof(buffer));
	}

	char soundFont[512];
	sscanf(buffer, "# Midi Settings\n soundfont = %s\n",
		soundFont);

	for (int32 i = 0; i < fListView->CountItems(); i++) {
		BStringItem* item = (BStringItem*)fListView->ItemAt(i);
		if (!strcmp(item->Text(), soundFont)) {
			fListView->Select(i);
			break;
		}
	}
}


void
MidiSettingsView::_SaveSettings()
{
	int32 selection = fListView->CurrentSelection();
	if (selection < 0)
		return;
	BStringItem* item = (BStringItem*)fListView->ItemAt(selection);
	if (item == NULL)
		return;

	char buffer[512];
	snprintf(buffer, 512, "# Midi Settings\n soundfont = %s\n",
			item->Text());

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(SETTINGS_FILE);
		BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		if (file.InitCheck() == B_OK)
			file.Write(buffer, strlen(buffer));
	}
}

