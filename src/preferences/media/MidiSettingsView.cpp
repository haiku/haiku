/*
 * Copyright 2014-2017, Haiku, Inc. All rights reserved.
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
#include <NodeMonitor.h>
#include <Path.h>
#include <PathFinder.h>
#include <ScrollView.h>
#include <StringList.h>
#include <StringView.h>

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Midi View"

const static uint32 kSelectSoundFont = 'SeSf';
const static uint32 kDoubleClick = 'DClk';


MidiSettingsView::MidiSettingsView()
	:
	SettingsView()
{
	BBox* defaultsBox = new BBox("SoundFonts");
	defaultsBox->SetLabel(B_TRANSLATE("SoundFonts"));
	BGridView* defaultsGridView = new BGridView();

	fListView = new BListView(B_SINGLE_SELECTION_LIST);
	fListView->SetSelectionMessage(new BMessage(kSelectSoundFont));
	fListView->SetInvocationMessage(new BMessage(kDoubleClick));

	BScrollView* scrollView = new BScrollView("ScrollView", fListView,
			0, false, true);
	BLayoutBuilder::Grid<>(defaultsGridView)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING)
		.Add(scrollView, 0, 0);

	defaultsBox->AddChild(defaultsGridView);
	fSoundFontStatus = new BStringView("SoundFontStatus", "");

	BLayoutBuilder::Group<>(this)
		.SetInsets(0, 0, 0, 0)
		.Add(defaultsBox)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fSoundFontStatus)
			.AddGlue()
			.End()
		.AddGlue();
}


/* virtual */
void
MidiSettingsView::AttachedToWindow()
{
	SettingsView::AttachedToWindow();

	fListView->SetTarget(this);
	_LoadSettings();
	_RetrieveSoundFontList();
	_SelectActiveSoundFont();
	_UpdateSoundFontStatus();
	_WatchFolders();
}


/* virtual */
void
MidiSettingsView::DetachedFromWindow()
{
	SettingsView::DetachedFromWindow();

	stop_watching(this);
}


/* virtual */
void
MidiSettingsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
		{
			int32 selected = fListView->CurrentSelection();
			BStringItem* olditem = (BStringItem*)fListView->ItemAt(selected);

			_RetrieveSoundFontList();

			int32 count = fListView->CountItems();
			if (count == 1) {
				fListView->Select(0);
				_SaveSettings();
			} else if (olditem != NULL) {
				for (int32 i = 0; i < fListView->CountItems(); i++) {
					BStringItem* item = (BStringItem*)fListView->ItemAt(i);
					if (strcmp(item->Text(), olditem->Text()) == 0) {
						fListView->Select(i);
						break;
					}
				}
			}
			_UpdateSoundFontStatus();
			break;
		}
		case kSelectSoundFont:
		{
			int selection = fListView->CurrentSelection();
			if (selection < 0)
				_SelectActiveSoundFont();
			else
				_SaveSettings();

			_UpdateSoundFontStatus();
			break;
		}
		case kDoubleClick:
		{
			int selection = fListView->CurrentSelection();
			if (selection < 0)
				break;

			BStringItem* item = (BStringItem*)fListView->ItemAt(selection);

			BEntry entry(item->Text());
			BEntry parent;
			entry.GetParent(&parent);
			entry_ref folderRef;
			parent.GetRef(&folderRef);
			BMessenger msgr("application/x-vnd.Be-TRAK");
			BMessage refMsg(B_REFS_RECEIVED);
			refMsg.AddRef("refs",&folderRef);
			msgr.SendMessage(&refMsg);
			break;
		}

		default:
			SettingsView::MessageReceived(message);
			break;
	}
}


void
MidiSettingsView::_RetrieveSoundFontList()
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
	if (BPrivate::read_midi_settings(&settings) == B_OK)
		fActiveSoundFont.SetTo(settings.soundfont_file);
}


void
MidiSettingsView::_SaveSettings()
{
	fActiveSoundFont = _SelectedSoundFont();
	if (fActiveSoundFont.Length() > 0) {
		struct BPrivate::midi_settings settings;
		strlcpy(settings.soundfont_file, fActiveSoundFont.String(),
			sizeof(settings.soundfont_file));
		BPrivate::write_midi_settings(settings);
	}
}


void
MidiSettingsView::_SelectActiveSoundFont()
{
	int32 count = fListView->CountItems();
	if (count == 1) {
		fListView->Select(0);
		_SaveSettings();
		return;
	}
	for (int32 i = 0; i < fListView->CountItems(); i++) {
		BStringItem* item = (BStringItem*)fListView->ItemAt(i);
		if (strcmp(item->Text(), fActiveSoundFont.String()) == 0) {
			fListView->Select(i);
			break;
		}
	}
}


BString
MidiSettingsView::_SelectedSoundFont() const
{
	BString string;
	int32 selection = fListView->CurrentSelection();
	if (selection >= 0) {
		BStringItem* item = (BStringItem*)fListView->ItemAt(selection);
		if (item != NULL)
			string = item->Text();
	}

	return string;
}


void
MidiSettingsView::_UpdateSoundFontStatus()
{
	if (fListView->IsEmpty()) {
		fSoundFontStatus->SetText(
			B_TRANSLATE("There are no SoundFont installed."));
		return;
	}
	int32 selection = fListView->CurrentSelection();
	if (selection < 0) {
		fSoundFontStatus->SetText(
			B_TRANSLATE("Please select a SoundFont."));
		return;
	}
	fSoundFontStatus->SetText("");
}


void
MidiSettingsView::_WatchFolders()
{
	BStringList paths;
	BPathFinder().FindPaths(B_FIND_PATH_DATA_DIRECTORY, "synth", paths);
	for (int32 i = 0; i < paths.CountStrings(); i++) {
		BEntry entry(paths.StringAt(i));
		node_ref nref;
		entry.GetNodeRef(&nref);

		watch_node(&nref, B_WATCH_ALL, this);
	}
}
