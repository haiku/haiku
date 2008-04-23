/*
 * Copyright 2008, Ralf Schülke, teammaui@web.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>

#include <Application.h>
#include <MessageRunner.h> 
#include <Button.h>
#include <Alert.h>
#include <TextView.h>
#include <String.h>

#include "Pairs.h"
#include "PairsGlobal.h"
#include "PairsWindow.h"
#include "PairsView.h"
#include "PairsTopButton.h"


PairsWindow::PairsWindow()
	: BWindow(BRect(100, 100, 405, 405), "Pairs", B_TITLED_WINDOW,
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
	fPairsView = new PairsView(Bounds().InsetByCopy(0, 0).OffsetToSelf(0, 0),
		"PairsView", B_FOLLOW_NONE);
	fPairsView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fPairsView);
}


PairsWindow::~PairsWindow()
{
	delete fPairComparing;
}


void
PairsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgCardButton:			
			if (fIsPairsActive) {
				fButtonClicks = fButtonClicks + 1;

				int32 num;
				if (message->FindInt32("ButtonNum", &num) < B_OK)
					break;

				//look what Icon is behind a button
				for (int h = 0; h < 16; h++) {
					if (fPairsView->GetIconFromPos(h) == num) {
						fPairCard = (h % 8);
						fButton = fPairsView->GetIconFromPos(h);
						break;
					} 
				}

				//gameplay
				fPairsView->fDeckCard[fButton]->Hide();

				if (fIsFirstClick) {
					fPairCardTmp = fPairCard;
					fButtonTmp = fButton;
				} else {
					delete fPairComparing;
						// message of message runner might not have arrived
						// yet, so it is deleted here to prevent any leaking
						// just in case
					fPairComparing = new BMessageRunner(BMessenger(this),
						new BMessage(kMsgPairComparing),  5 * 100000L, 1);
					fIsPairsActive = false;
				}

				fIsFirstClick = !fIsFirstClick;
			}		
			break;
			
			case kMsgPairComparing:
				delete fPairComparing;
				fPairComparing = NULL;

				fIsPairsActive = true;

				if (fPairCard == fPairCardTmp) {
					fFinishPairs++;
				} else {
					fPairsView->fDeckCard[fButton]->Show();
					fPairsView->fDeckCard[fButtonTmp]->Show();
// TODO: remove when app_server is fixed: (Could this be the Firefox problem?)
fPairsView->Invalidate();
				}

				//game end and results
				if (fFinishPairs == 8) {
					BString strAbout;
					strAbout 
						<< "Pairs\n"
						<< "\twritten by Ralf Schülke\n"
						<< "\tCopyright 2008, Haiku Inc.\n"
						<< "\n"
						<< "You completed the game in " << fButtonClicks + 1
						<< " clicks.\n";
						
					BAlert* alert = new BAlert("about", strAbout.String(), "New game",
						"Quit game");
				
					BTextView* view = alert->TextView();
					BFont font;

					view->SetStylable(true);

					view->GetFont(&font);
					font.SetSize(18);
					font.SetFace(B_BOLD_FACE);
					view->SetFontAndColor(0, 6, &font);
					view->ResizeToPreferred();
				
					if (alert->Go() == 0) {
						//New game
						fButtonClicks = 0;
						fFinishPairs = 0;
						fPairsView->CreateGameBoard();
					} else {
						//Quit game
						be_app->PostMessage(B_QUIT_REQUESTED);
					}
				}
				break;
			
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


