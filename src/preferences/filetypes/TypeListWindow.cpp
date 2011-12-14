/*
 * Copyright 2006-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "MimeTypeListView.h"
#include "TypeListWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <ScrollView.h>

#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Type List Window"


const uint32 kMsgTypeSelected = 'tpsl';
const uint32 kMsgSelected = 'seld';


TypeListWindow::TypeListWindow(const char* currentType, uint32 what,
		BWindow* target)
	:
	BWindow(BRect(100, 100, 360, 440), B_TRANSLATE("Choose type"),
		B_MODAL_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fTarget(target),
	fWhat(what)
{
	float padding = be_control_look->DefaultItemSpacing();

	fSelectButton = new BButton("select", B_TRANSLATE("Done"),
		new BMessage(kMsgSelected));
	fSelectButton->SetEnabled(false);

	BButton* button = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(B_CANCEL));

	fSelectButton->MakeDefault(true);

	fListView = new MimeTypeListView("typeview", NULL, true, false);
	fListView->SetSelectionMessage(new BMessage(kMsgTypeSelected));
	fListView->SetInvocationMessage(new BMessage(kMsgSelected));

	BScrollView* scrollView = new BScrollView("scrollview", fListView,
		B_FRAME_EVENTS | B_WILL_DRAW, false, true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, padding)
		.SetInsets(padding)
		.Add(scrollView)
		.AddGroup(B_HORIZONTAL, padding)
			.Add(button)
			.Add(fSelectButton);

	BAlignment buttonAlignment =
		BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_VERTICAL_CENTER);
	button->SetExplicitAlignment(buttonAlignment);
	fSelectButton->SetExplicitAlignment(buttonAlignment);

	MoveTo(target->Frame().LeftTop() + BPoint(15.0f, 15.0f));
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

