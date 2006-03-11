/*
 * Copyright 2002-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Oliver Siebenmarck
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Michael Wilber
 */


#include "DataTranslations.h"
#include "DataTranslationsMessages.h"
#include "DataTranslationsSettings.h"
#include "DataTranslationsWindow.h"
#include "IconView.h"
#include "TranslatorListView.h"

#include <Application.h>
#include <Screen.h>
#include <Alert.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <TextView.h>
#include <TranslationDefs.h>
#include <TranslatorRoster.h>

#define DTW_RIGHT	400
#define DTW_BOTTOM	300


DataTranslationsWindow::DataTranslationsWindow()
	: BWindow(BRect(0, 0, DTW_RIGHT, DTW_BOTTOM),
		"DataTranslations", B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	MoveTo(dynamic_cast<DataTranslationsApplication *>(be_app)->WindowCorner());

	// Make sure that the window isn't positioned off screen
	BScreen screen;
	BRect scrf = screen.Frame(), winf = Frame();
	float x = winf.left, y = winf.top;
	scrf.top += 24;
	scrf.left += 5;
	scrf.right -= 5;
	if (winf.left < scrf.left)
		x = scrf.left;
	if (winf.right > scrf.right)
		x = scrf.right - winf.Width() - 5;
	if (winf.top < scrf.top)
		y = scrf.top;
	if (winf.bottom > scrf.bottom)
		y = scrf.bottom - winf.Height() - 15;

	if (x != winf.left || y != winf.top)
		MoveTo(x, y);

	_SetupViews();
	Show();
}


DataTranslationsWindow::~DataTranslationsWindow()
{
}


// Reads the installed translators and adds them to our BListView
status_t
DataTranslationsWindow::_PopulateListView()
{
	BTranslatorRoster *roster = BTranslatorRoster::Default();

	// Get all Translators on the system. Gives us the number of translators
	// installed in num_translators and a reference to the first one
	int32 numTranslators;
	translator_id *translators = NULL;
	roster->GetAllTranslators(&translators, &numTranslators);

	for (int32 i = 0; i < numTranslators; i++) {
		// Getting the first three Infos: Name, Info & Version
		int32 version;
		const char *name, *info;
		roster->GetTranslatorInfo(translators[i], &name, &info, &version);
		fTranslatorListView->AddItem(new BStringItem(name));
	}

	delete[] translators;
	return B_OK;
}


status_t
DataTranslationsWindow::_GetTranslatorInfo(int32 id, const char *&tranName,
	const char *&tranInfo, int32 &tranVersion, BPath &tranPath)
{
	// Returns information about the translator with the given id

	if (id < 0)
		return B_BAD_VALUE;

	int32 numTranslators = 0;
	translator_id tid = 0, *translators = NULL;
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	roster->GetAllTranslators(&translators, &numTranslators);
	tid = ((id < numTranslators) ? translators[id] : 0);
	delete[] translators;

	if (id >= numTranslators)
		return B_BAD_VALUE;

	// Getting the first three Infos: Name, Info & Version
	roster->GetTranslatorInfo(tid, &tranName, &tranInfo, &tranVersion);
	
	// Get the translator's path
	entry_ref tranRef;
	roster->GetRefFor(tid, &tranRef);
	BEntry tmpEntry(&tranRef);
	tranPath.SetTo(&tmpEntry);
	
	return B_OK;
}


status_t
DataTranslationsWindow::_ShowConfigView(int32 id)
{
	// Shows the config panel for the translator with the given id
	
	if (id < 0)
		return B_BAD_VALUE;

	int32 numTranslators = 0;
	translator_id tid = 0, *translators = NULL;
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	roster->GetAllTranslators(&translators, &numTranslators);
	tid = ((id < numTranslators) ? translators[id] : 0);
	delete[] translators;

	if (id >= numTranslators)
		return B_BAD_VALUE;
	
	// fConfigView is NULL the first time this function
	// is called, prevent a segment fault
	if (fConfigView)
		fRightBox->RemoveChild(fConfigView);
	BMessage emptyMsg;
	BRect rect(0, 0, 200, 233);
	status_t ret = roster->MakeConfigurationView(tid, &emptyMsg, &fConfigView, &rect);
	if (ret != B_OK) {
		fRightBox->RemoveChild(fConfigView);
		return ret;
	}

	BRect configRect(fRightBox->Bounds());
	configRect.InsetBy(3, 3);
	configRect.bottom -= 45;
	float width = 0, height = 0;
	fConfigView->GetPreferredSize(&width, &height);
	float widen = max_c(0, width - configRect.Width());
	float heighten = max_c(0, height - configRect.Height());
	if (widen > 0 || heighten > 0) {
		ResizeBy(widen, heighten);
		configRect.right += widen;
		configRect.bottom += heighten;
	}
	fConfigView->MoveTo(configRect.left, configRect.top);
	fConfigView->ResizeTo(configRect.Width(), configRect.Height());
	fConfigView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		// force config views to all have the same color
	fRightBox->AddChild(fConfigView);
	
	UpdateIfNeeded();
	
	return B_OK;
}


void
DataTranslationsWindow::_SetupViews()
{
	fConfigView = NULL;
	// This is NULL until a translator is
	// selected from the listview

	// Window box
	BView* mainView = new BView(Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);
	mainView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(mainView);

	// Add the translators list view
	fTranslatorListView = new TranslatorListView(BRect(10, 10, 120, Bounds().Height() - 10),
		"TransList", B_SINGLE_SELECTION_LIST); 
	fTranslatorListView->SetSelectionMessage(new BMessage(SEL_CHANGE));

	BScrollView *scrollView = new BScrollView("scroll_trans", fTranslatorListView,
    		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW | B_FRAME_EVENTS,
    		false, true, B_FANCY_BORDER);
	mainView->AddChild(scrollView);

	// Box around the config and info panels
	fRightBox = new BBox(BRect(130 + B_V_SCROLL_BAR_WIDTH, 10, Bounds().Width() - 10,
		Bounds().Height() - 10), "Right_Side", B_FOLLOW_ALL);
	mainView->AddChild(fRightBox);

	// Add the translator icon view
	BRect rightRect(fRightBox->Bounds()), iconRect(0, 0, 31, 31);
	rightRect.InsetBy(8, 8);
	iconRect.OffsetTo(rightRect.left, rightRect.bottom - iconRect.Height());
	fIconView = new IconView(iconRect, "Icon",
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_FRAME_EVENTS);
	fRightBox->AddChild(fIconView);

	// Add the translator info button
	BRect infoRect(0, 0, 80, 20);
	BButton *button = new BButton(infoRect, "STD", "Info...",
		new BMessage(BUTTON_MSG), B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	button->ResizeToPreferred();
	button->MoveTo(fRightBox->Bounds().Width() - button->Bounds().Width() - 10.0f,
		fRightBox->Bounds().Height() - button->Bounds().Height() - 10.0f);
	fRightBox->AddChild(button);

	// Add the translator name view
	BRect tranNameRect(iconRect.right + 5, iconRect.top,
		infoRect.left - 5, iconRect.bottom);
	fTranslatorNameView = new BStringView(tranNameRect, "TranName", "None",
    		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fRightBox->AddChild(fTranslatorNameView);

	// Populate the translators list view   
	_PopulateListView();

	fTranslatorListView->MakeFocus();
	fTranslatorListView->Select(0, false);    
}


bool
DataTranslationsWindow::QuitRequested()
{
	BPoint pt(Frame().left, Frame().top);
	dynamic_cast<DataTranslationsApplication *>(be_app)->SetWindowCorner(pt);
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
	message << "Name:\t" << name << "\nVersion:\t";

	// Convert the version number into a readable format
	message << (int)B_TRANSLATION_MAJOR_VERSION(version)
		<< '.' << (int)B_TRANSLATION_MINOR_VERSION(version)
		<< '.' << (int)B_TRANSLATION_REVISION_VERSION(version);
	message << "\nInfo:\t\t" << info <<
		"\n\nPath:\n" << path.Path() << "\n";

	BAlert *alert = new BAlert("info", message.String(), "OK");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetFace(B_BOLD_FACE);

	const char* labels[] = {"Name:", "Version:", "Info:", "Path:", NULL};
	for (int32 i = 0; labels[i]; i++) {
		int32 index = message.FindFirst(labels[i]);
		view->SetFontAndColor(index, index + strlen(labels[i]), &font);
	}

	alert->Go();
}


void
DataTranslationsWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case BUTTON_MSG:
		{
			int32 selected = fTranslatorListView->CurrentSelection(0);
			if (selected < 0) {
				// If no translator is selected, show a message explaining
				// what the config panel is for
				(new BAlert("Panel Info",
					"Translation Settings\n\n"
					"Use this control panel to set values that various\n"
					"translators use when no other settings are specified\n"
					"in the application.",
					"OK"))->Go();
				break;
			}

			_ShowInfoAlert(selected);
			break;
		}

		case SEL_CHANGE:
		{
			// Update the icon and translator info panel
			// to match the new selection

			int32 selected = fTranslatorListView->CurrentSelection(0);
			if (selected < 0) {
				// If none selected, clear the old one
				fIconView->DrawIcon(false);
				fTranslatorNameView->SetText("");
				fRightBox->RemoveChild(fConfigView);
				break;
			}

			_ShowConfigView(selected);

			const char* name = NULL;
			const char* info = NULL;
			int32 version = 0;
			BPath path;
			_GetTranslatorInfo(selected, name, info, version, path);
			fTranslatorNameView->SetText(path.Leaf());
			fIconView->SetIcon(path);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}	
}

