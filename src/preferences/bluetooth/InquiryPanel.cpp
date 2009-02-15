/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <Alert.h>
#include <Button.h>
#include <GroupLayoutBuilder.h>
#include <StatusBar.h>
#include <SpaceLayoutItem.h>
#include <TextView.h>
#include <TabView.h>

#include <InquiryPanel.h>
#include "defs.h"

static const uint32 kMsgUpdate = 'dflt';
static const uint32 kMsgRevert = 'rvrt';

static const uint32 kMsgStartServices = 'SrSR';
static const uint32 kMsgStopServices = 'StST';
static const uint32 kMsgShowDebug = 'ShDG';


InquiryPanel::InquiryPanel(BRect frame)
 :	BWindow(frame, "Bluetooth", B_TITLED_WINDOW,
 		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS,
 		B_ALL_WORKSPACES)
{
	BRect iDontCare(0,0,0,0);
	BRect iDontCareToo(0,0,5,5);

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	fScanProgress = new BStatusBar(iDontCare, "status", "Scanning", "Scan time");
	fScanProgress->SetMaxValue(52);

	fMessage = new BTextView(iDontCare, "description",
		iDontCare2, B_FOLLOW_LEFT_RIGHT,
		B_WILL_DRAW | B_FRAME_EVENTS);
	fMessage->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fMessage->SetLowColor(fMessage->ViewColor());
	fMessage->MakeEditable(false);
	fMessage->SetText("asdfdasas asdfas asdfasd a dfad asdf dfasdf a");

	fInquiryButton = new BButton("Inquiry", "Inquiry",
		new BMessage(kMsgRevert), B_WILL_DRAW);
		
	fAddButton = new BButton("ad", "Add device to list",
		new BMessage(kMsgRevert), B_WILL_DRAW);		


	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(fScanProgress)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(5))
		.Add(fMessage)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(5))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
			.Add(fAddButton)
			.AddGlue()
			.Add(fInquiryButton)
		)
		.SetInsets(5, 5, 5, 5)
	);
}


void
InquiryPanel::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgUpdate:
/*			fDefaultsButton->SetEnabled(fRemoteDevices->IsDefaultable()
								|| fAntialiasingSettings->IsDefaultable());

			fRevertButton->SetEnabled(true);*/
			break;
/*		case kMsgSetDefaults:
			fColorsView -> MessageReceived(new BMessage(DEFAULT_SETTINGS));
			fAntialiasingSettings->SetDefaults();
			fDefaultsButton->SetEnabled(false);
			fRevertButton->SetEnabled(true);
			break;

		case kMsgRevert:
			fColorsView -> MessageReceived(new BMessage(REVERT_SETTINGS));
			fAntialiasingSettings->Revert();
			fDefaultsButton->SetEnabled(fColorsView->IsDefaultable()
								|| fAntialiasingSettings->IsDefaultable());
			fRevertButton->SetEnabled(false);
			break;
*/		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
InquiryPanel::QuitRequested(void)
{

	return true;
}
