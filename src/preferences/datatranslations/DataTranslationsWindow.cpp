/*
 * Copyright 2002-2017, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Oliver Siebenmarck
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Michael Wilber
 *		Maxime Simon
 */


#include "DataTranslationsWindow.h"

#include <algorithm>

#include <math.h>
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
#include <SupportDefs.h>
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
	BWindow(BRect(0.0f, 0.0f, 597.0f, 368.0f),
		B_TRANSLATE_SYSTEM_NAME("DataTranslations"),
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE
			| B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fRelease(NULL)
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

	float maxWidth = 0;

	for (int32 i = 0; i < numTranslators; i++) {
		// Getting the first three Infos: Name, Info & Version
		int32 version;
		const char* name;
		const char* info;
		roster->GetTranslatorInfo(translators[i], &name, &info, &version);
		fTranslatorListView->AddItem(new TranslatorItem(translators[i], name));
		maxWidth = std::max(maxWidth, fTranslatorListView->StringWidth(name));
	}

	fTranslatorListView->SortItems();

	fTranslatorListView->SetExplicitSize(BSize(maxWidth + 20, B_SIZE_UNSET));

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

	if (fConfigView != NULL) {
		fRightBox->RemoveChild(fConfigView);
		delete fConfigView;
		fConfigView = NULL;
		fInfoText = NULL;
		if (fRelease != NULL) {
			fRelease->Release();
			fRelease = NULL;
		}
	}

	BMessage emptyMessage;
	BRect rect(0.0f, 0.0f, 200.0f, 233.0f);
	status_t result = roster->MakeConfigurationView(id, &emptyMessage,
		&fConfigView, &rect);

	if (result != B_OK)
		return result;

	fConfigView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
		// force config views to all have the same color
	fRightBox->AddChild(fConfigView);

	// for default 12pt font size: 597 ≈ (0.85 * 12 * 49)
	fConfigView->SetExplicitMinSize(
		BSize(ceilf(be_control_look->DefaultItemSpacing() * 49)
			- fTranslatorListView->Frame().Width(), B_SIZE_UNSET));

	// Make sure the translator's image doesn't get unloaded while we are still
	// showing a config view whose code is in the image
	fRelease = roster->AcquireTranslator(id);

	return B_OK;
}


void
DataTranslationsWindow::_ShowInfoView()
{
	if (fConfigView != NULL) {
		fRightBox->RemoveChild(fConfigView);
		delete fConfigView;
		fConfigView = NULL;
		fInfoText = NULL;
		if (fRelease != NULL) {
			fRelease->Release();
			fRelease = NULL;
		}
	}

	fInfoText = new BTextView("info text");
	fInfoText->MakeEditable(false);
	fInfoText->MakeSelectable(false);
	fInfoText->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fInfoText->SetText(B_TRANSLATE(
		"Use this control panel to set default values for translators, "
		"to be used when no other settings are specified by an application."));
	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
	fInfoText->SetFontAndColor(be_plain_font, B_FONT_ALL, &textColor);

	BGroupView* group = new BGroupView(B_VERTICAL);
	group->AddChild(fInfoText);
	float spacing = be_control_look->DefaultItemSpacing();
	group->GroupLayout()->SetInsets(spacing, spacing, spacing, spacing);
	fRightBox->AddChild(group);
	fConfigView = group;

	fConfigView->SetExplicitMinSize(
		BSize(ceilf(spacing * be_plain_font->Size() * 0.7)
			- fTranslatorListView->Frame().Width(),
			ceilf(spacing * be_plain_font->Size() * 0.4)));
}


void
DataTranslationsWindow::_SetupViews()
{
	fInfoText = NULL;
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
		new BMessage(kMsgTranslatorInfo),
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	fButton->SetEnabled(false);

	// Populate the translators list view
	_PopulateListView();

	// Build the layout
	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(scrollView, 3)
		.AddGroup(B_VERTICAL)
			.Add(fRightBox)
			.AddGroup(B_HORIZONTAL)
				.Add(fIconView)
				.AddGlue()
				.Add(fButton)
				.End()
			.End()
		.End();

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

	const char* labels[] = { B_TRANSLATE("Name:"), B_TRANSLATE("Version:"),
		B_TRANSLATE("Info:"), B_TRANSLATE("Path:"), NULL };
	int offsets[4];

	BString message;
	BString temp;

	offsets[0] = 0;
	temp.SetToFormat("%s %s\n", labels[0], name);

	message.Append(temp);

	offsets[1] = message.Length();
	// Convert the version number into a readable format
	temp.SetToFormat("%s %" B_PRId32 ".%" B_PRId32 ".%" B_PRId32 "\n\n", labels[1],
		B_TRANSLATION_MAJOR_VERSION(version),
		B_TRANSLATION_MINOR_VERSION(version),
		B_TRANSLATION_REVISION_VERSION(version));

	message.Append(temp);

	offsets[2] = message.Length();
	temp.SetToFormat("%s\n%s\n\n", labels[2], info);

	message.Append(temp);

	offsets[3] = message.Length();
	temp.SetToFormat("%s %s\n", labels[3], path.Path());

	message.Append(temp);

	BAlert* alert = new BAlert(B_TRANSLATE("Info"), message.String(),
		B_TRANSLATE("OK"));
	BTextView* view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetFace(B_BOLD_FACE);

	for (int32 i = 0; labels[i]; i++) {
		view->SetFontAndColor(offsets[i], offsets[i] + strlen(labels[i]), &font);
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

		case B_COLORS_UPDATED:
		{
			if (fInfoText == NULL || fInfoText->Parent() == NULL)
				break;

			rgb_color color;
			if (message->FindColor(ui_color_name(B_PANEL_TEXT_COLOR), &color)
					== B_OK) {
				fInfoText->SetFontAndColor(be_plain_font, B_FONT_ALL, &color);
			}
			break;
		}

		case B_TRANSLATOR_ADDED:
		{
			int32 index = 0;
			int32 id;
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
			int32 index = 0;
			int32 id;
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
