/*
 * Copyright 2002-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Oliver Siebenmarck
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Michael Wilber
 *		Maxime Simon
 */


#include "DataTranslationsWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <Alignment.h>
#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Entry.h>
#include <GroupView.h>
#include <IconView.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <TextView.h>
#include <TranslationDefs.h>
#include <TranslatorRoster.h>

#include "DataTranslations.h"
#include "DataTranslationsSettings.h"
#include "TranslatorListView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DataTranslations"


const uint32 kMsgTranslatorInfo = 'trin';
const uint32 kMsgSelectedTranslator = 'trsl';


DataTranslationsWindow::DataTranslationsWindow()
	:
	BWindow(BRect(0, 0, 550, 350), B_TRANSLATE_SYSTEM_NAME("DataTranslations"),
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE
		| B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	MoveTo(DataTranslationsSettings::Instance()->WindowCorner());

	_SetupViews();

	// Make sure that the window isn't positioned off screen
	BScreen screen;
	BRect screenFrame = screen.Frame();
	if (!screenFrame.Contains(Frame()))
		CenterOnScreen();

	BTranslatorRoster* roster = BTranslatorRoster::Default();
	roster->StartWatching(this);

	Show();
}


DataTranslationsWindow::~DataTranslationsWindow()
{
	BTranslatorRoster* roster = BTranslatorRoster::Default();
	roster->StopWatching(this);
}


// Reads the installed translators and adds them to our BListView
status_t
DataTranslationsWindow::_PopulateListView()
{
	BTranslatorRoster* roster = BTranslatorRoster::Default();

	// Get all Translators on the system. Gives us the number of translators
	// installed in num_translators and a reference to the first one
	int32 numTranslators;
	translator_id* translators = NULL;
	roster->GetAllTranslators(&translators, &numTranslators);

	for (int32 i = 0; i < numTranslators; i++) {
		// Getting the first three Infos: Name, Info & Version
		int32 version;
		const char* name;
		const char* info;
		roster->GetTranslatorInfo(translators[i], &name, &info, &version);
		fTranslatorListView->AddItem(new TranslatorItem(translators[i], name));
	}

	delete[] translators;
	return B_OK;
}


status_t
DataTranslationsWindow::_GetTranslatorInfo(int32 id, const char*& name,
	const char*& info, int32& version, BPath& path)
{
	// Returns information about the translator with the given id

	if (id < 0)
		return B_BAD_VALUE;

	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (roster->GetTranslatorInfo(id, &name, &info, &version) != B_OK)
		return B_ERROR;

	// Get the translator's path
	entry_ref ref;
	if (roster->GetRefFor(id, &ref) == B_OK) {
		BEntry entry(&ref);
		path.SetTo(&entry);
	} else
		path.Unset();

	return B_OK;
}


status_t
DataTranslationsWindow::_ShowConfigView(int32 id)
{
	// Shows the config panel for the translator with the given id

	if (id < 0)
		return B_BAD_VALUE;

	BTranslatorRoster* roster = BTranslatorRoster::Default();

	if (fConfigView) {
		fRightBox->RemoveChild(fConfigView);
		delete fConfigView;
		fConfigView = NULL;
	}

	BMessage emptyMsg;
	BRect rect(0, 0, 200, 233);
	status_t ret = roster->MakeConfigurationView(id, &emptyMsg,
		&fConfigView, &rect);

	if (ret != B_OK)
		return ret;

	fConfigView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		// force config views to all have the same color
	fRightBox->AddChild(fConfigView);

	return B_OK;
}


void
DataTranslationsWindow::_ShowInfoView()
{
	if (fConfigView) {
		fRightBox->RemoveChild(fConfigView);
		delete fConfigView;
		fConfigView = NULL;
	}

	BTextView* view = new BTextView("info text");
	view->MakeEditable(false);
	view->MakeSelectable(false);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	view->SetText(B_TRANSLATE(
		"Use this control panel to set default values for translators, "
		"to be used when no other settings are specified by an application."));

	BGroupView* group = new BGroupView(B_VERTICAL);
	group->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	group->AddChild(view);
	float spacing = be_control_look->DefaultItemSpacing();
	group->GroupLayout()->SetInsets(spacing, spacing, spacing, spacing);
	fRightBox->AddChild(group);
	fConfigView = group;
}


void
DataTranslationsWindow::_SetupViews()
{
	fConfigView = NULL;
	// This is NULL until a translator is
	// selected from the listview

	// Add the translators list view
	fTranslatorListView = new TranslatorListView("TransList");
	fTranslatorListView->SetSelectionMessage(
		new BMessage(kMsgSelectedTranslator));

	BScrollView* scrollView = new BScrollView("scroll_trans",
		fTranslatorListView, B_WILL_DRAW | B_FRAME_EVENTS, false,
		true, B_FANCY_BORDER);

	// Box around the config and info panels
	fRightBox = new BBox("Right_Side");
	fRightBox->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_USE_FULL_HEIGHT));

	// Add the translator icon view
	fIconView = new IconView();

	// Add the translator info button
	fButton = new BButton("info", B_TRANSLATE("Info"),
		new BMessage(kMsgTranslatorInfo), B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	fButton->SetEnabled(false);

	// Populate the translators list view
	_PopulateListView();

	// Build the layout
	float padding = be_control_look->DefaultItemSpacing();
	BLayoutBuilder::Group<>(this, B_HORIZONTAL, padding)
		.SetInsets(padding, padding, padding, padding)
		.Add(scrollView, 3)
		.AddGrid(padding, padding, 6)
			.SetInsets(0, 0, 0, 0)
			.Add(fRightBox, 0, 0, 3, 1)
			.Add(fIconView, 0, 1)
			.Add(fButton, 2, 1);

	fTranslatorListView->MakeFocus();
	_ShowInfoView();
}


bool
DataTranslationsWindow::QuitRequested()
{
	BPoint pt(Frame().LeftTop());
	DataTranslationsSettings::Instance()->SetWindowCorner(pt);
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
DataTranslationsWindow::_ShowInfoAlert(int32 id)
{
	const char* name = NULL;
	const char* info = NULL;
	BPath path;
	int32 version = 0;
	_GetTranslatorInfo(id, name, info, version, path);

	BString message;
	// Convert the version number into a readable format
	snprintf(message.LockBuffer(2048), 2048,
		B_TRANSLATE("Name: %s \nVersion: %ld.%ld.%ld\n\n"
			"Info:\n%s\n\nPath:\n%s\n"),
		name, B_TRANSLATION_MAJOR_VERSION(version),
		B_TRANSLATION_MINOR_VERSION(version),
		B_TRANSLATION_REVISION_VERSION(version), info, path.Path());
	message.UnlockBuffer();

	BAlert* alert = new BAlert(B_TRANSLATE("Info"), message.String(),
		B_TRANSLATE("OK"));
	BTextView* view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetFace(B_BOLD_FACE);

	const char* labels[] = { B_TRANSLATE("Name:"), B_TRANSLATE("Version:"),
		B_TRANSLATE("Info:"), B_TRANSLATE("Path:"), NULL };
	for (int32 i = 0; labels[i]; i++) {
		int32 index = message.FindFirst(labels[i]);
		view->SetFontAndColor(index, index + strlen(labels[i]), &font);
	}

	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}


void
DataTranslationsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgTranslatorInfo:
		{
			int32 selected = fTranslatorListView->CurrentSelection(0);
			if (selected < 0)
				break;

			TranslatorItem* item = fTranslatorListView->TranslatorAt(selected);
			if (item != NULL)
				_ShowInfoAlert(item->ID());
			break;
		}

		case kMsgSelectedTranslator:
		{
			// Update the icon and translator info panel
			// to match the new selection

			int32 selected = fTranslatorListView->CurrentSelection(0);
			if (selected < 0) {
				// If none selected, clear the old one
				fIconView->DrawIcon(false);
				fButton->SetEnabled(false);
				fRightBox->RemoveChild(fConfigView);
				_ShowInfoView();
				break;
			}

			TranslatorItem* item = fTranslatorListView->TranslatorAt(selected);
			if (item == NULL)
				break;

			_ShowConfigView(item->ID());

			const char* name = NULL;
			const char* info = NULL;
			int32 version = 0;
			BPath path;
			_GetTranslatorInfo(item->ID(), name, info, version, path);
			fIconView->SetIcon(path);
			fButton->SetEnabled(true);
			break;
		}

		case B_TRANSLATOR_ADDED:
		{
			int32 index = 0, id;
			while (message->FindInt32("translator_id", index++, &id) == B_OK) {
				const char* name;
				const char* info;
				int32 version;
				BPath path;
				if (_GetTranslatorInfo(id, name, info, version, path) == B_OK)
					fTranslatorListView->AddItem(new TranslatorItem(id, name));
			}

			fTranslatorListView->SortItems();
			break;
		}

		case B_TRANSLATOR_REMOVED:
		{
			int32 index = 0, id;
			while (message->FindInt32("translator_id", index++, &id) == B_OK) {
				for (int32 i = 0; i < fTranslatorListView->CountItems(); i++) {
					TranslatorItem* item = fTranslatorListView->TranslatorAt(i);

					if (item == NULL)
						continue;

					if (item->ID() == (translator_id)id) {
						fTranslatorListView->RemoveItem(i);
						delete item;
						break;
					}
				}
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}
