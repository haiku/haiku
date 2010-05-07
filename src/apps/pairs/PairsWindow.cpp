/*
 * Copyright 2008 Ralf Schülke, ralf.schuelke@googlemail.com.
 * Copyright 2010 Adam Smith <adamd.smith@utoronto.ca>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PairsWindow.h"

#include <stdio.h>

#include <Application.h>
#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <String.h>
#include <TextView.h>

#include "Pairs.h"
#include "PairsGlobal.h"
#include "PairsView.h"
#include "PairsTopButton.h"


// #pragma mark - PairsWindow


#undef TR_CONTEXT
#define TR_CONTEXT "PairsWindow"

const uint32 MENU_NEW					= 'MGnw';
const uint32 MENU_SIZE					= 'MGsz';
const uint32 MENU_QUIT					= 'MGqu';


PairsWindow::PairsWindow()
	:
	BWindow(BRect(100, 100, 405, 423), TR("Pairs"), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE
			| B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fPairComparing(NULL),
	fIsFirstClick(true),
	fIsPairsActive(true),
	fPairCard(0),
	fPairCardTmp(0),
	fButtonTmp(0),
	fButton(0),
	fButtonClicks(0),
	fFinishPairs(0)
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

	BMenu* menu = new BMenu(TR("Game"));
	fMenuBar->AddItem(menu);

	BMenuItem* menuItem;
	menu->AddItem(menuItem = new BMenuItem(TR("New"),
		new BMessage(MENU_NEW), 'N'));

	menu->AddSeparatorItem();

	BMenu* sizeMenu = new BMenu(TR("Size"));
	sizeMenu->SetRadioMode(true);

	BMessage* sizeMessage = new BMessage(MENU_SIZE);
	sizeMessage->AddInt32("width", 4);
	sizeMessage->AddInt32("height", 4);
	sizeMenu->AddItem(menuItem = new BMenuItem(TR("Beginner (4x4)"),
		sizeMessage));
	menuItem->SetMarked(true);

	sizeMessage = new BMessage(MENU_SIZE);
	sizeMessage->AddInt32("width", 6);
	sizeMessage->AddInt32("height", 6);
	sizeMenu->AddItem(menuItem = new BMenuItem(TR("Intermediate (6x6)"),
		sizeMessage));

	sizeMessage = new BMessage(MENU_SIZE);
	sizeMessage->AddInt32("width", 8);
	sizeMessage->AddInt32("height", 8);
	sizeMenu->AddItem(menuItem = new BMenuItem(TR("Expert (8x8)"),
		sizeMessage));

	menu->AddItem(sizeMenu);

	menu->AddSeparatorItem();

	menu->AddItem(menuItem = new BMenuItem(TR("Quit"),
		new BMessage(MENU_QUIT), 'Q'));
}


void
PairsWindow::_MakeGameView(int width, int height)
{
	BRect viewBounds = Bounds();
	viewBounds.top = fMenuBar->Bounds().Height() + 1;

	fPairsView = new PairsView(viewBounds, "PairsView", width, height,
		B_FOLLOW_NONE);
	fPairsView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fPairsView);
}


void
PairsWindow::NewGame()
{
	fButtonClicks = 0;
	fFinishPairs = 0;
	fPairsView->CreateGameBoard();
}


void
PairsWindow::SetGameSize(int width, int height)
{
	ResizeTo((kBitmapSize + kSpaceSize) * width + kSpaceSize,
		(kBitmapSize + kSpaceSize) * height + kSpaceSize
		+ fMenuBar->Bounds().Height());
	RemoveChild(fPairsView);
	delete fPairsView;
	_MakeGameView(width, height);
	NewGame();
}


void
PairsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MENU_NEW:
			NewGame();
			break;
		case MENU_SIZE:
		{
			int32 width;
			int32 height;
			if (message->FindInt32("width", &width) == B_OK
				&& message->FindInt32("height", &height) == B_OK) {
				SetGameSize(width, height);
			}
			break;
		}
		case MENU_QUIT:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		case kMsgCardButton:
			if (fIsPairsActive) {
				fButtonClicks++;

				int32 num;
				if (message->FindInt32("ButtonNum", &num) < B_OK)
					break;

				// look what Icon is behind a button
				for (int h = 0; h < fPairsView->fNumOfCards; h++) {
					if (fPairsView->GetIconFromPos(h) == num) {
						fPairCard = (h % fPairsView->fNumOfCards / 2);
						fButton = fPairsView->GetIconFromPos(h);
						break;
					}
				}

				// gameplay
				((TopButton*)fPairsView->fDeckCard.ItemAt(fButton))->Hide();

				if (fIsFirstClick) {
					fPairCardTmp = fPairCard;
					fButtonTmp = fButton;
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
			}
			break;

			case kMsgPairComparing:
				delete fPairComparing;
				fPairComparing = NULL;

				fIsPairsActive = true;

				if (fPairCard == fPairCardTmp)
					fFinishPairs++;
				else {
					((TopButton*)fPairsView->fDeckCard.ItemAt(fButton))->Show();
					((TopButton*)fPairsView->fDeckCard.ItemAt(fButtonTmp))->Show();
				}

				// game end and results
				if (fFinishPairs == fPairsView->fNumOfCards / 2) {
					BString strAbout;
					BString score;
					score << fButtonClicks;
					strAbout
						<< TR("Pairs\n"
						"\twritten by Ralf Schülke\n"
						"\tCopyright 2008-2010, Haiku Inc.\n"
						"\n"
						"You completed the game in %s clicks.\n");
						
					strAbout.Replace("%s", score.String(), 1);

					BAlert* alert = new BAlert("about", TR(strAbout.String()),
						TR("New game"), TR("Quit game"));

					BTextView* view = alert->TextView();
					BFont font;

					view->SetStylable(true);

					view->GetFont(&font);
					font.SetSize(18);
					font.SetFace(B_BOLD_FACE);
					view->SetFontAndColor(0, 6, &font);
					view->ResizeToPreferred();

					if (alert->Go() == 0) {
						// New game
						NewGame();
					} else {
						// Quit game
						be_app->PostMessage(B_QUIT_REQUESTED);
					}
				}
				break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}
