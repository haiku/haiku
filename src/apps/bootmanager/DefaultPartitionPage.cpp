/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "DefaultPartitionPage.h"


#include <Catalog.h>
#include <ControlLook.h>
#include <Locale.h>
#include <Message.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <Slider.h>
#include <string.h>
#include <String.h>
#include <TextView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DefaultPartitionPage"


enum {
	kMsgPartition = 'part',
	kMsgTimeout = 'time'
};


// The timeout code to wait indefinitely
// Note: The timeout is encoded in seconds, -1 indicates to wait indefinitely
const int32 kTimeoutIndefinitely = -1;
const int32 kDefaultTimeout = kTimeoutIndefinitely;

struct TimeoutOption {
	int32 timeout;
	const char* label;
};

static const TimeoutOption gTimeoutOptions[] = {
	{ 0, B_TRANSLATE_MARK("Immediately")},
	{ 1, B_TRANSLATE_MARK("After one second")},
	{ 2, B_TRANSLATE_MARK("After two seconds")},
	{ 3, B_TRANSLATE_MARK("After three seconds")},
	{ 4, B_TRANSLATE_MARK("After four seconds")},
	{ 5, B_TRANSLATE_MARK("After five seconds")},
	{ 60, B_TRANSLATE_MARK("After one minute")},
	{ kTimeoutIndefinitely, B_TRANSLATE_MARK("Never")}
};


#define kNumberOfTimeoutOptions \
	(int32)(sizeof(gTimeoutOptions) / sizeof(TimeoutOption))


static int32
get_index_for_timeout(int32 timeout)
{
	int32 defaultIndex = 0;
	for (int32 i = 0; i < kNumberOfTimeoutOptions; i ++) {
		if (gTimeoutOptions[i].timeout == timeout)
			return i;

		if (gTimeoutOptions[i].timeout == kDefaultTimeout)
			defaultIndex = i;
	}
	return defaultIndex;
}


static int32
get_timeout_for_index(int32 index)
{
	if (index < 0)
		return gTimeoutOptions[0].timeout;
	if (index >= kNumberOfTimeoutOptions)
		return gTimeoutOptions[kNumberOfTimeoutOptions-1].timeout;
	return gTimeoutOptions[index].timeout;
}


const char*
get_label_for_timeout(int32 timeout)
{
	int32 index = get_index_for_timeout(timeout);
	return gTimeoutOptions[index].label;
}


// #pragma mark -


DefaultPartitionPage::DefaultPartitionPage(BMessage* settings, BRect frame,
	const char* name)
	:
	WizardPageView(settings, frame, name, B_FOLLOW_ALL,
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
	fTimeoutSlider->SetTarget(this);
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
			break;
		}
		case kMsgTimeout:
		{
			int32 sliderValue = fTimeoutSlider->Value();
			int32 timeout = get_timeout_for_index(sliderValue);
			fSettings->ReplaceInt32("timeout", timeout);

			BString label;
			_GetTimeoutLabel(timeout, label);
			fTimeoutSlider->SetLabel(label.String());
			break;
		}

		default:
			WizardPageView::MessageReceived(msg);
	}
}


void
DefaultPartitionPage::_BuildUI()
{
	const float kTextDistance = be_control_look->DefaultItemSpacing();
	BRect rect(Bounds());

	BString text;
	text << B_TRANSLATE_COMMENT("Default Partition", "Title") << "\n"
		<< B_TRANSLATE("Please specify a default partition and a timeout.\n"
			"The boot menu will load the default partition after "
			"the timeout unless you select another partition. You "
			"can also have the boot menu wait indefinitely for you "
			"to select a partition.\n"
			"Keep the 'ALT' key pressed to disable the timeout at boot time.");

	fDescription = CreateDescription(rect, "description", text);
	MakeHeading(fDescription);
	AddChild(fDescription);
	LayoutDescriptionVertically(fDescription);
	rect.top = fDescription->Frame().bottom + kTextDistance;

	BPopUpMenu* popUpMenu = _CreatePopUpMenu();
	fDefaultPartition = new BMenuField(rect, "partitions",
		B_TRANSLATE_COMMENT("Default Partition:", "Menu field label"),
		popUpMenu);
	float divider = be_plain_font->StringWidth(fDefaultPartition->Label()) + 3;
	fDefaultPartition->SetDivider(divider);
	AddChild(fDefaultPartition);
	fDefaultPartition->ResizeToPreferred();

	// timeout slider
	rect.top = fDefaultPartition->Frame().bottom + kTextDistance;
	int32 timeout;
	fSettings->FindInt32("timeout", &timeout);
	BString timeoutLabel;
	_GetTimeoutLabel(timeout, timeoutLabel);

	int32 sliderValue = get_index_for_timeout(timeout);

	fTimeoutSlider = new BSlider(rect, "timeout", timeoutLabel.String(),
		new BMessage(kMsgTimeout), 0, kNumberOfTimeoutOptions-1,
		B_BLOCK_THUMB,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fTimeoutSlider->SetModificationMessage(new BMessage(kMsgTimeout));
	fTimeoutSlider->SetValue(sliderValue);
	fTimeoutSlider->SetLimitLabels(B_TRANSLATE("Immediately"),
		B_TRANSLATE("Never"));
	fTimeoutSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fTimeoutSlider->SetHashMarkCount(kNumberOfTimeoutOptions);
	fTimeoutSlider->ResizeToPreferred();
	AddChild(fTimeoutSlider);

	_Layout();
}


BPopUpMenu*
DefaultPartitionPage::_CreatePopUpMenu()
{
	int32 defaultPartitionIndex;
	fSettings->FindInt32("defaultPartition", &defaultPartitionIndex);

	BMenuItem* selectedItem = NULL;
	int32 selectedItemIndex = 0;

	BPopUpMenu* menu = new BPopUpMenu(B_TRANSLATE_COMMENT("Partitions",
		"Pop up menu title"));
	BMessage message;
	for (int32 i = 0; fSettings->FindMessage("partition", i, &message) == B_OK;
			i++) {
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


void
DefaultPartitionPage::_GetTimeoutLabel(int32 timeout, BString& label)
{
	const char* text = B_TRANSLATE_NOCOLLECT(get_label_for_timeout(timeout));
	label = B_TRANSLATE("Timeout: %s");
	label.ReplaceFirst("%s", text);
}


void
DefaultPartitionPage::_Layout()
{
	LayoutDescriptionVertically(fDescription);

	const float kTextDistance = be_control_look->DefaultItemSpacing();
	float left = fDefaultPartition->Frame().left;
	float top = fDescription->Frame().bottom + kTextDistance;

	fDefaultPartition->MoveTo(left, top);
	top = fDefaultPartition->Frame().bottom + kTextDistance;

	fTimeoutSlider->MoveTo(left, top);
}
