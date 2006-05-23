/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MimeTypeListView.h"
#include "TypeListWindow.h"

#include <Button.h>
#include <ScrollView.h>

#include <string.h>


const uint32 kMsgTypeSelected = 'tpsl';
const uint32 kMsgSelected = 'seld';


TypeListWindow::TypeListWindow(const char* currentType, uint32 what, BWindow* target)
	: BWindow(BRect(100, 100, 360, 440), "Choose Type", B_MODAL_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS),
	fTarget(target),
	fWhat(what)
{
	BRect rect = Bounds();
	BView* topView = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	fSelectButton = new BButton(rect, "select", "Done",
		new BMessage(kMsgSelected), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fSelectButton->ResizeToPreferred();
	fSelectButton->MoveTo(topView->Bounds().right - 8.0f - fSelectButton->Bounds().Width(),
		topView->Bounds().bottom - 8.0f - fSelectButton->Bounds().Height());
	fSelectButton->SetEnabled(false);
	topView->AddChild(fSelectButton);

	BButton* button = new BButton(fSelectButton->Frame(), "cancel", "Cancel",
		new BMessage(B_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->MoveBy(-button->Bounds().Width() - 8.0f, 0.0f);
	topView->AddChild(button);

	fSelectButton->MakeDefault(true);

	rect.bottom = button->Frame().top - 10.0f;
	rect.top = 10.0f;
	rect.left = 10.0f;
	rect.right -= 10.0f + B_V_SCROLL_BAR_WIDTH;
	fListView = new MimeTypeListView(rect, "typeview", NULL, true, false,
		B_FOLLOW_ALL);
	fListView->SetSelectionMessage(new BMessage(kMsgTypeSelected));
	fListView->SetInvocationMessage(new BMessage(kMsgSelected));

	BScrollView* scrollView = new BScrollView("scrollview", fListView,
		B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	topView->AddChild(scrollView);

	MoveTo(target->Frame().LeftTop() + BPoint(15.0f, 15.0f));
	SetSizeLimits(button->Bounds().Width() + fSelectButton->Bounds().Width() + 64.0f, 32767.0f,
		fSelectButton->Bounds().Height() * 5.0f, 32767.0f);
}


TypeListWindow::~TypeListWindow()
{
}


void
TypeListWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgTypeSelected:
			fSelectButton->SetEnabled(fListView->CurrentSelection() >= 0);
			break;

		case kMsgSelected:
		{
			MimeTypeItem* item = dynamic_cast<MimeTypeItem*>(fListView->ItemAt(
				fListView->CurrentSelection()));
			if (item != NULL) {
				BMessage select(fWhat);
				select.AddString("type", item->Type());
				fTarget.SendMessage(&select);
			}

			// supposed to fall through
		}
		case B_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

