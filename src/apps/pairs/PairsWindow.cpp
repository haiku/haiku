/*
 * Copyright 2008 Ralf Schülke, ralf.schuelke@googlemail.com.
 * Copyright 2010 Adam Smith <adamd.smith@utoronto.ca>
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ralf Schülke, ralf.schuelke@googlemail.com
 *		John Scipione, jscipione@gmail.com
 *		Adam Smith, adamd.smith@utoronto.ca
 */


#include "PairsWindow.h"

#include <Application.h>
#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <ObjectList.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <String.h>
#include <StringFormat.h>
#include <TextView.h>

#include "Pairs.h"
#include "PairsButton.h"
#include "PairsView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PairsWindow"


const uint32 MENU_NEW					= 'MGnw';
const uint32 MENU_DIFFICULTY			= 'MGdf';
const uint32 MENU_QUIT					= 'MGqu';
const uint32 MENU_ICON_SIZE				= 'MSIs';

const uint32 kMsgPairComparing			= 'pcom';


//	#pragma mark - PairsWindow


PairsWindow::PairsWindow()
	:
	BWindow(BRect(0, 0, 0, 0), B_TRANSLATE_SYSTEM_NAME("Pairs"),
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE
			| B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fPairComparing(NULL),
	fIsFirstClick(true),
	fIsPairsActive(true),
	fPairCardPosition(0),
	fPairCardTmpPosition(0),
	fButtonTmpPosition(0),
	fButtonPosition(0),
	fButtonClicks(0),
	fFinishPairs(0),
	fIconSizeMenu(NULL)
{
	_MakeMenuBar();
	_MakeGameView(4, 4);

	CenterOnScreen();
}


PairsWindow::~PairsWindow()
{
	delete fPairComparing;
}


void
PairsWindow::_MakeMenuBar()
{
	fMenuBar = new BMenuBar(BRect(0, 0, 0, 0), "menubar");
	AddChild(fMenuBar);

	BMenu* gameMenu = new BMenu(B_TRANSLATE("Game"));
	fMenuBar->AddItem(gameMenu);

	BMenuItem* menuItem;

	BMenu* newMenu = new BMenu(B_TRANSLATE("New"));
	newMenu->SetRadioMode(true);

	BMessage* difficultyMessage = new BMessage(MENU_DIFFICULTY);
	difficultyMessage->AddInt32("rows", 4);
	difficultyMessage->AddInt32("cols", 4);
	newMenu->AddItem(menuItem = new BMenuItem(B_TRANSLATE("Beginner (4x4)"),
		difficultyMessage));
	menuItem->SetMarked(true);

	difficultyMessage = new BMessage(MENU_DIFFICULTY);
	difficultyMessage->AddInt32("rows", 6);
	difficultyMessage->AddInt32("cols", 6);
	newMenu->AddItem(new BMenuItem(B_TRANSLATE("Intermediate (6x6)"),
		difficultyMessage));

	difficultyMessage = new BMessage(MENU_DIFFICULTY);
	difficultyMessage->AddInt32("rows", 8);
	difficultyMessage->AddInt32("cols", 8);
	newMenu->AddItem(new BMenuItem(B_TRANSLATE("Expert (8x8)"),
		difficultyMessage));

	menuItem = new BMenuItem(newMenu, new BMessage(MENU_NEW));
	menuItem->SetShortcut('N', B_COMMAND_KEY);
	gameMenu->AddItem(menuItem);

	gameMenu->AddSeparatorItem();

	gameMenu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(MENU_QUIT), 'Q'));

	fIconSizeMenu = new BMenu(B_TRANSLATE("Size"));
	fIconSizeMenu->SetRadioMode(true);
	fMenuBar->AddItem(fIconSizeMenu);

	BMessage* iconSizeMessage = new BMessage(MENU_ICON_SIZE);
	iconSizeMessage->AddInt32("size", kSmallIconSize);
	fIconSizeMenu->AddItem(new BMenuItem(
		B_TRANSLATE("Small"), iconSizeMessage), 0);

	iconSizeMessage = new BMessage(MENU_ICON_SIZE);
	iconSizeMessage->AddInt32("size", kMediumIconSize);
	fIconSizeMenu->AddItem(menuItem = new BMenuItem(
		B_TRANSLATE("Medium"), iconSizeMessage), 1);
	menuItem->SetMarked(true);

	iconSizeMessage = new BMessage(MENU_ICON_SIZE);
	iconSizeMessage->AddInt32("size", kLargeIconSize);
	fIconSizeMenu->AddItem(new BMenuItem(
		B_TRANSLATE("Large"), iconSizeMessage), 2);
}


void
PairsWindow::_MakeGameView(uint8 rows, uint8 cols)
{
	BRect viewBounds = Bounds();
	viewBounds.top = fMenuBar->Bounds().Height() + 1;

	uint8 iconSize;
	BMenuItem* marked = fIconSizeMenu->FindMarked();
	if (marked != NULL) {
		switch (fIconSizeMenu->IndexOf(marked)) {
			case 0:
				iconSize = kSmallIconSize;
				break;

			case 2:
				iconSize = kLargeIconSize;
				break;

			case 1:
			default:
				iconSize = kMediumIconSize;
		}
	} else {
		iconSize = kMediumIconSize;
		fIconSizeMenu->ItemAt(1)->SetMarked(true);
	}

	fPairsView = new PairsView(viewBounds, "PairsView", rows, cols, iconSize);
	AddChild(fPairsView);
	_ResizeWindow(rows, cols);
}


void
PairsWindow::NewGame()
{
	fButtonClicks = 0;
	fFinishPairs = 0;
	fPairsView->CreateGameBoard();
}


void
PairsWindow::SetGameSize(uint8 rows, uint8 cols)
{
	RemoveChild(fPairsView);
	delete fPairsView;

	_MakeGameView(rows, cols);
	NewGame();
}


void
PairsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MENU_NEW:
			NewGame();
			break;

		case MENU_DIFFICULTY:
		{
			int32 rows;
			int32 cols;
			if (message->FindInt32("rows", &rows) == B_OK
				&& message->FindInt32("cols", &cols) == B_OK) {
				SetGameSize(rows, cols);
			}
			break;
		}

		case MENU_ICON_SIZE:
		{
			int32 size;
			if (message->FindInt32("size", &size) == B_OK) {
				fPairsView->SetIconSize(size);
				_ResizeWindow(fPairsView->Rows(), fPairsView->Cols());
			}

			break;
		}

		case MENU_QUIT:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case kMsgCardButton:
		{
			if (!fIsPairsActive)
				break;

			int32 buttonNumber;
			if (message->FindInt32("button number", &buttonNumber) != B_OK)
				break;

			BObjectList<PairsButton>* pairsButtonList
				= fPairsView->PairsButtonList();
			if (pairsButtonList == NULL)
				break;

			// look at what icon is behind a button
			int32 buttonCount = pairsButtonList->CountItems();
			for (int32 i = 0; i < buttonCount; i++) {
				int32 iconPosition = fPairsView->GetIconPosition(i);
				if (iconPosition == buttonNumber) {
					fPairCardPosition = i % (buttonCount / 2);
					fButtonPosition = iconPosition;
					break;
				}
			}

			// gameplay
			fButtonClicks++;
			pairsButtonList->ItemAt(fButtonPosition)->Hide();

			if (fIsFirstClick) {
				fPairCardTmpPosition = fPairCardPosition;
				fButtonTmpPosition = fButtonPosition;
			} else {
				delete fPairComparing;
					// message of message runner might not have arrived
					// yet, so it is deleted here to prevent any leaking
					// just in case
				BMessage message(kMsgPairComparing);
				fPairComparing = new BMessageRunner(BMessenger(this),
					&message,  5 * 100000L, 1);
				fIsPairsActive = false;
			}

			fIsFirstClick = !fIsFirstClick;
			break;
		}

		case kMsgPairComparing:
		{
			BObjectList<PairsButton>* pairsButtonList
				= fPairsView->PairsButtonList();
			if (pairsButtonList == NULL)
				break;

			delete fPairComparing;
			fPairComparing = NULL;

			fIsPairsActive = true;

			if (fPairCardPosition == fPairCardTmpPosition)
				fFinishPairs++;
			else {
				pairsButtonList->ItemAt(fButtonPosition)->Show();
				pairsButtonList->ItemAt(fButtonTmpPosition)->Show();
			}

			// game end and results
			if (fFinishPairs == pairsButtonList->CountItems() / 2) {
				BString strAbout = B_TRANSLATE("%app%\n"
					"\twritten by Ralf Schülke\n"
					"\tCopyright 2008-2010, Haiku Inc.\n"
					"\n");

				strAbout.ReplaceFirst("%app%",
					B_TRANSLATE_SYSTEM_NAME("Pairs"));

				// Note: in english the singular form is never used, but other
				// languages behave differently.
				static BStringFormat format(B_TRANSLATE(
					"You completed the game in "
					"{0, plural, one{# click} other{# clicks}}.\n"));
				format.Format(strAbout, fButtonClicks);

				BAlert* alert = new BAlert("about",
					strAbout.String(),
					B_TRANSLATE("New game"),
					B_TRANSLATE("Quit game"));

				BTextView* view = alert->TextView();
				BFont font;

				view->SetStylable(true);

				view->GetFont(&font);
				font.SetSize(18);
				font.SetFace(B_BOLD_FACE);
				view->SetFontAndColor(0,
					strlen(B_TRANSLATE_SYSTEM_NAME("Pairs")), &font);
				view->ResizeToPreferred();
				alert->SetShortcut(0, B_ESCAPE);

				if (alert->Go() == 0)
					NewGame();
				else
					be_app->PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}


//	#pragma mark - PairsWindow private methods


void
PairsWindow::_ResizeWindow(uint8 rows, uint8 cols)
{
	int32 iconSize = fPairsView->IconSize();
	int32 spacing = fPairsView->Spacing();

	ResizeTo((iconSize + spacing) * rows + spacing,
		(iconSize + spacing) * cols + spacing + fMenuBar->Bounds().Height());
}
