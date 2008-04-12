/*
 * Copyright 2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "DefaultPartitionPage.h"


#include <Message.h>

#include <MenuItem.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <TextView.h>

#include <string.h>
#include <String.h>


const uint32 kMsgPartition = 'part';
const uint32 kMsgTimeout = 'time';


DefaultPartitionPage::DefaultPartitionPage(BMessage* settings, BRect frame, const char* name)
	: WizardPageView(settings, frame, name, B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
{
	_BuildUI();
}


DefaultPartitionPage::~DefaultPartitionPage()
{
}


void
DefaultPartitionPage::FrameResized(float width, float height)
{
	WizardPageView::FrameResized(width, height);
	_Layout();
}


void
DefaultPartitionPage::AttachedToWindow()
{
	fDefaultPartition->Menu()->SetTargetForItems(this);
	fWait0->SetTarget(this);
	fWait5->SetTarget(this);
	fWait10->SetTarget(this);
	fWait15->SetTarget(this);
}


void
DefaultPartitionPage::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kMsgPartition:
			{
				int32 index;
				msg->FindInt32("index", &index);
				fSettings->ReplaceInt32("defaultPartition", index);
			}
			break;
		case kMsgTimeout:
			{
				int32 timeout;
				msg->FindInt32("timeout", &timeout);
				fSettings->ReplaceInt32("timeout", timeout);
			}
			break;
		default:
			WizardPageView::MessageReceived(msg);
	}
}


static const float kTextDistance = 10;

void
DefaultPartitionPage::_BuildUI()
{
	BRect rect(Bounds());
	
	fDescription = CreateDescription(rect, "description", 
		"Default Partition\n\n"
		"Please specify a default partition and a timeout.\n"
		"The boot menu will load the default partition after "
		"the timeout unless you select another partition. You "
		"can also have the boot menu wait indefinitely for you "
		"to select a partition."
		);
	MakeHeading(fDescription);
	AddChild(fDescription);
	LayoutDescriptionVertically(fDescription);
	rect.top = fDescription->Frame().bottom + kTextDistance;
	
	BPopUpMenu* popUpMenu = _CreatePopUpMenu();	
	fDefaultPartition = new BMenuField(rect, "partitions", "Default Partition:", 
		popUpMenu);
	float divider = be_plain_font->StringWidth(fDefaultPartition->Label()) + 3;
	fDefaultPartition->SetDivider(divider);
	AddChild(fDefaultPartition);
	fDefaultPartition->ResizeToPreferred();
	
	rect.top = fDefaultPartition->Frame().bottom + kTextDistance;
	int32 timeout;
	fSettings->FindInt32("timeout", &timeout);
	fWait0 = _CreateWaitRadioButton(rect, "wait0", "Wait Indefinitely", -1, timeout);
	fWait5 = _CreateWaitRadioButton(rect, "wait5", "Wait 5 Seconds", 5, timeout);
	fWait10 = _CreateWaitRadioButton(rect, "wait10", "Wait 10 Seconds", 10, timeout);
	fWait15 = _CreateWaitRadioButton(rect, "wait15", "Wait 15 Seconds", 15, timeout);
	
	_Layout();
}


BPopUpMenu*
DefaultPartitionPage::_CreatePopUpMenu()
{
	int32 defaultPartitionIndex;
	fSettings->FindInt32("defaultPartition", &defaultPartitionIndex);
	
	BMenuItem* selectedItem = NULL;
	int32 selectedItemIndex = 0;
	
	BPopUpMenu* menu = new BPopUpMenu("Partitions");
	BMessage message;
	for (int32 i = 0; fSettings->FindMessage("partition", i, &message) == B_OK; i ++) {
		
		bool show;
		if (message.FindBool("show", &show) != B_OK || !show)
			continue;
		
		BString name;
		message.FindString("name", &name);

		BMessage* msg = new BMessage(kMsgPartition);
		msg->AddInt32("index", i);
		BMenuItem* item = new BMenuItem(name.String(), msg);
		menu->AddItem(item);
		if (defaultPartitionIndex == i || selectedItem == NULL) {
			selectedItem = item;
			selectedItemIndex = i;
		}
	}
	fSettings->ReplaceInt32("defaultPartition", selectedItemIndex);
	selectedItem->SetMarked(true);
	return menu;
}


BRadioButton* 
DefaultPartitionPage::_CreateWaitRadioButton(BRect rect, const char* name, const char* label, int32 timeout, 
	int32 defaultTimeout)
{
	BMessage* msg = new BMessage(kMsgTimeout);
	msg->AddInt32("timeout", timeout);
	BRadioButton* button = new BRadioButton(rect, name, label, msg);
	AddChild(button);
	button->ResizeToPreferred();

	if (timeout == defaultTimeout)
		button->SetValue(1);
		
	return button;
}


void
DefaultPartitionPage::_Layout()
{
	LayoutDescriptionVertically(fDescription);
	
	float left = fDefaultPartition->Frame().left;
	float top = fDescription->Frame().bottom + kTextDistance;
	
	BView* controls[] = {
		fDefaultPartition,
		fWait0,
		fWait5,
		fWait10,
		fWait15
	};
	
	for (int i = 0; i < 5; i ++) {
		BView* view = controls[i];
		view->MoveTo(left, top);
		top = view->Frame().bottom + 3;
	}
}

