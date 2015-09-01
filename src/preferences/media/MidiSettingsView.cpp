/*
 * Copyright 2014, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "MidiSettingsView.h"

#include <MidiSettings.h>

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
#include <PathFinder.h>
#include <ScrollView.h>
#include <StringList.h>

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Midi View"

const static uint32 kSelectSoundFont = 'SeSf';


MidiSettingsView::MidiSettingsView()
	:
	SettingsView()
{
	BBox* defaultsBox = new BBox("SoundFont");
	defaultsBox->SetLabel(B_TRANSLATE("Available SoundFonts"));
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
	// TODO: Duplicated code between here
	// and BSoftSynth::SetDefaultInstrumentsFile
	BStringList paths;
	status_t status = BPathFinder::FindPaths(B_FIND_PATH_DATA_DIRECTORY,
			"synth", paths);
	if (status != B_OK)
		return;

	fListView->MakeEmpty();

	for (int32 i = 0; i < paths.CountStrings(); i++) {
		BDirectory directory(paths.StringAt(i).String());
		BEntry entry;
		if (directory.InitCheck() != B_OK)
			continue;
		while (directory.GetNextEntry(&entry) == B_OK) {
			BNode node(&entry);
			BNodeInfo nodeInfo(&node);
			char mimeType[B_MIME_TYPE_LENGTH];
			// TODO: For some reason the mimetype check fails.
			// maybe because the file hasn't yet been sniffed and recognized?
			if (nodeInfo.GetType(mimeType) == B_OK
				/*&& !strcmp(mimeType, "audio/x-soundfont")*/) {
				BPath fullPath = paths.StringAt(i).String();
				fullPath.Append(entry.Name());
				fListView->AddItem(new BStringItem(fullPath.Path()));
			}
		}
	}
}


void
MidiSettingsView::_LoadSettings()
{
	struct BPrivate::midi_settings settings;
	if (BPrivate::read_midi_settings(&settings) == B_OK) {
		for (int32 i = 0; i < fListView->CountItems(); i++) {
			BStringItem* item = (BStringItem*)fListView->ItemAt(i);
			if (!strcmp(item->Text(), settings.soundfont_file)) {
				fListView->Select(i);
				break;
			}
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

	struct BPrivate::midi_settings settings;
	strlcpy(settings.soundfont_file, item->Text(), sizeof(settings.soundfont_file));
	BPrivate::write_midi_settings(settings);
}

