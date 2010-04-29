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


#include "DataTranslations.h"
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

#include "Area.h"
#include "BALMLayout.h"
#include "OperatorType.h"
#include "XTab.h"
#include "YTab.h"


const uint32 kMsgTranslatorInfo = 'trin';
const uint32 kMsgSelectedTranslator = 'trsl';


DataTranslationsWindow::DataTranslationsWindow()
	: BWindow(BRect(0, 0, 550, 350), "DataTranslations", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS)
{
	MoveTo(dynamic_cast<DataTranslationsApplication *>(be_app)->WindowCorner());

	_SetupViews();

	// Make sure that the window isn't positioned off screen
	BScreen screen;
	BRect screenFrame = screen.Frame();
	if (!screenFrame.Contains(Frame()))
		CenterOnScreen();

	BTranslatorRoster *roster = BTranslatorRoster::Default();
	roster->StartWatching(this);

	Show();
}


DataTranslationsWindow::~DataTranslationsWindow()
{
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	roster->StopWatching(this);
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

	BTranslatorRoster *roster = BTranslatorRoster::Default();
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

	BTranslatorRoster *roster = BTranslatorRoster::Default();

	// fConfigView is NULL the first time this function
	// is called, prevent a segment fault
	if (fConfigView)
		fRightBox->RemoveChild(fConfigView);

	BMessage emptyMsg;
	BRect rect(0, 0, 200, 233);
	status_t ret = roster->MakeConfigurationView(id, &emptyMsg, &fConfigView, &rect);
	if (ret != B_OK) {
		fRightBox->RemoveChild(fConfigView);
		return ret;
	}

	BRect configRect(fRightBox->Bounds());
	configRect.InsetBy(3, 3);
	configRect.bottom -= 45;
	float width = 0, height = 0;
	if ((fConfigView->Flags() & B_SUPPORTS_LAYOUT) != 0) {
		BSize configSize = fConfigView->ExplicitPreferredSize();
		width = configSize.Width();
		height = configSize.Height();
	} else {
		fConfigView->GetPreferredSize(&width, &height);
	}
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
	fTranslatorListView = new TranslatorListView(BRect(0, 0, 1, 1), "TransList",
		B_SINGLE_SELECTION_LIST);
	fTranslatorListView->SetSelectionMessage(new BMessage(kMsgSelectedTranslator));

	BScrollView *scrollView = new BScrollView("scroll_trans", fTranslatorListView,
    		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW | B_FRAME_EVENTS,
    		false, true, B_FANCY_BORDER);

	// Box around the config and info panels
	fRightBox = new BBox("Right_Side");

	// Add the translator icon view
	fIconView = new IconView(BRect(0, 0, 31, 31), "Icon",
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_FRAME_EVENTS);

	// Add the translator info button
	BButton *button = new BButton("STD", "Info" B_UTF8_ELLIPSIS,
		new BMessage(kMsgTranslatorInfo), B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);

	// Add the translator name view
	fTranslatorNameView = new BStringView("TranName", "None");

	// Populate the translators list view
	_PopulateListView();

	// Build the layout
	BALMLayout *layout = new BALMLayout();
	mainView->SetLayout(layout);

	XTab *x1 = layout->AddXTab();
	XTab *x2 = layout->AddXTab();
	XTab *x3 = layout->AddXTab();
	YTab *y1 = layout->AddYTab();

	Area *leftArea = layout->AddArea(layout->Left(), layout->Top(),
		x1, layout->Bottom(), scrollView);
	leftArea->SetLeftInset(10);
	leftArea->SetTopInset(10);
	leftArea->SetBottomInset(10);

	Area *rightArea = layout->AddArea(x1, layout->Top(), layout->Right(), y1, fRightBox);
	rightArea->SetLeftInset(10);
	rightArea->SetTopInset(10);
	rightArea->SetRightInset(10);
	rightArea->SetBottomInset(10);

	Area *iconArea = layout->AddArea(x1, y1, x2, layout->Bottom(), fIconView);
	iconArea->SetLeftInset(10);
	iconArea->SetBottomInset(10);

	Area *infoButtonArea = layout->AddArea(x3, y1, layout->Right(), layout->Bottom(), button);
	infoButtonArea->SetRightInset(10);
	infoButtonArea->SetBottomInset(10);

	layout->AddConstraint(3.0, x1, -1.0, layout->Right(), OperatorType(EQ), 0.0);

	fTranslatorListView->MakeFocus();
	fTranslatorListView->Select(0);
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
	message << "Name: " << name << "\nVersion: ";

	// Convert the version number into a readable format
	message << (int)B_TRANSLATION_MAJOR_VERSION(version)
		<< '.' << (int)B_TRANSLATION_MINOR_VERSION(version)
		<< '.' << (int)B_TRANSLATION_REVISION_VERSION(version);
	message << "\nInfo: " << info <<
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
		case kMsgTranslatorInfo:
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

			TranslatorItem* item = dynamic_cast<TranslatorItem*>(fTranslatorListView->ItemAt(selected));
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
				fTranslatorNameView->SetText("");
				fRightBox->RemoveChild(fConfigView);
				break;
			}

			TranslatorItem* item = dynamic_cast<TranslatorItem*>(fTranslatorListView->ItemAt(selected));
			if (item == NULL)
				break;

			_ShowConfigView(item->ID());

			const char* name = NULL;
			const char* info = NULL;
			int32 version = 0;
			BPath path;
			_GetTranslatorInfo(item->ID(), name, info, version, path);
			fTranslatorNameView->SetText(path.Leaf());
			fIconView->SetIcon(path);
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
					TranslatorItem* item = dynamic_cast<TranslatorItem*>(fTranslatorListView->ItemAt(i));
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

