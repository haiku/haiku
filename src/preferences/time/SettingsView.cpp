/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Julun <host.haiku@gmx.de>
 */

#include "SettingsView.h"
#include "TimeMessages.h"


#include <CheckBox.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <RadioButton.h>


const uint32 kMsgMilTime = 'MilT';
const uint32 kMsgShowSeconds = 'ShSc';


const uint32 kMsgIsoDate = 'IDat';
const uint32 kMsgEuroDate = 'EDat';
const uint32 kMonthDayYear = '_mdy';
const uint32 kSeparatorChanged = '_tsc';
const uint32 kTrackerDateFormatChanged = 'TDFC';


const char* kDeskbarSignature = "application/x-vnd.Be-TSKB";
const char* kTrackerSignature = "application/x-vnd.Be-TRAK";


SettingsView::SettingsView(BRect frame)
	: BView(frame, "Settings", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP),
	  fInitialized(false),
	  fRecentDateWhat(kMonthDayYear)
{
}


SettingsView::~SettingsView()
{
}


void
SettingsView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	if (!fInitialized) {
		_InitView();
		fInitialized = true;
	}
}


void
SettingsView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgMilTime:
		case kMsgIsoDate:
			_UpdateTimeSettings(message->what);
			break;

		case kMsgEuroDate:
		case kMonthDayYear:
		case kMsgShowSeconds:
			_UpdateDateSettings(message->what);
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
SettingsView::_InitView()
{
	BRect bounds(Bounds());
	bounds.InsetBy(10.0, 10.0);

	BRect frameLeft(Bounds());
	frameLeft.right = frameLeft.Width() / 2;
	frameLeft.InsetBy(10.0f, 10.0f);

	BStringView *timeSettings = new BStringView(frameLeft, "time", "Time:");
	AddChild(timeSettings);
	timeSettings->ResizeToPreferred();

	frameLeft.OffsetBy(10.0, timeSettings->Bounds().Height() + 5.0);
	fShow24Hours = new BCheckBox(frameLeft, "show24Hours", "24 Hour Clock"
		, new BMessage(kMsgMilTime));
	AddChild(fShow24Hours);
	fShow24Hours->SetTarget(this);
	fShow24Hours->ResizeToPreferred();
	
	frameLeft.OffsetBy(0.0, fShow24Hours->Bounds().Height() + 5.0);
	fShowSeconds = new BCheckBox(frameLeft, "showSeconds", "Show Seconds"
		, new BMessage(kMsgShowSeconds));
	AddChild(fShowSeconds);
	fShowSeconds->SetTarget(this);
	fShowSeconds->ResizeToPreferred();

	frameLeft.OffsetBy(0.0, 2 * fShowSeconds->Bounds().Height() + 5.0);
	fGmtTime = new BRadioButton(frameLeft, "gmtTime", "Set clock to GMT"
		, new BMessage(kRTCUpdate));
	AddChild(fGmtTime);
	fGmtTime->ResizeToPreferred();

	frameLeft.OffsetBy(0.0, fGmtTime->Bounds().Height() + 5.0);
	fLocalTime = new BRadioButton(frameLeft, "local", "Set clock to local time",
		new BMessage(kRTCUpdate));
	AddChild(fLocalTime);
	fLocalTime->ResizeToPreferred();
	fLocalTime->SetValue(B_CONTROL_ON);

	BRect frameRight(Bounds());
	frameRight.left = frameRight.Width() / 2;
	frameRight.InsetBy(10.0f, 10.0f);

	BView *dateView = new BView(frameRight, "dateView", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP);
	AddChild(dateView);
	if (Parent())
		dateView->SetViewColor(Parent()->ViewColor());

	frameRight.OffsetTo(B_ORIGIN);
	BStringView *dateSettings = new BStringView(frameRight, "date", "Date:");
	dateView->AddChild(dateSettings);
	dateSettings->ResizeToPreferred();

	frameRight.OffsetBy(10.0, dateSettings->Bounds().Height() + 5.0);
	fYearMonthDay = new BRadioButton(frameRight, "yearMonthDay", "Year-Month-Day"
		, new BMessage(kMsgIsoDate));
	dateView->AddChild(fYearMonthDay);
	fYearMonthDay->SetTarget(this);
	fYearMonthDay->ResizeToPreferred();

	frameRight.OffsetBy(0.0, fYearMonthDay->Bounds().Height() + 5.0);
	fDayMonthYear = new BRadioButton(frameRight, "dayMonthYear", "Day-Month-Year"
		, new BMessage(kMsgEuroDate));
	dateView->AddChild(fDayMonthYear);
	fDayMonthYear->SetTarget(this);
	fDayMonthYear->ResizeToPreferred();

	frameRight.OffsetBy(0.0, fDayMonthYear->Bounds().Height() + 5.0);
	fMonthDayYear = new BRadioButton(frameRight, "monthDayYear", "Month-Day-Year"
		, new BMessage(kMonthDayYear));
	dateView->AddChild(fMonthDayYear);
	fMonthDayYear->SetTarget(this);
	fMonthDayYear->ResizeToPreferred();
	fMonthDayYear->SetValue(B_CONTROL_ON);

	frameRight.OffsetBy(0.0, fMonthDayYear->Bounds().Height() + 5.0);
	fStartWeekMonday = new BCheckBox(frameRight, "startWeekMonday"
		, "Start week with Monday", new BMessage(kWeekStart));
	dateView->AddChild(fStartWeekMonday);
	fStartWeekMonday->SetTarget(this);
	fStartWeekMonday->ResizeToPreferred();

	fDateSeparator = new BPopUpMenu("Separator");
	fDateSeparator->AddItem(new BMenuItem("-", new BMessage(kSeparatorChanged)));
	BMenuItem *item = new BMenuItem("/", new BMessage(kSeparatorChanged));
	fDateSeparator->AddItem(item);
	item->SetMarked(true);
	fDateSeparator->AddItem(new BMenuItem("\\", new BMessage(kSeparatorChanged)));
	fDateSeparator->AddItem(new BMenuItem(".", new BMessage(kSeparatorChanged)));
	fDateSeparator->AddItem(new BMenuItem("None", new BMessage(kSeparatorChanged)));
	fDateSeparator->AddItem(new BMenuItem("Space", new BMessage(kSeparatorChanged)));
	
	frameRight.OffsetBy(0.0, fStartWeekMonday->Bounds().Height() + 5.0);
	BMenuField *menuField = new BMenuField(frameRight, "regions", "Date separator:"
		, fDateSeparator, false);
	dateView->AddChild(menuField);
	menuField->ResizeToPreferred();
}


void
SettingsView::_UpdateDateSettings(const uint32 _what)
{
	uint32 what = _what;
	if (_what == kMonthDayYear)
		what = fRecentDateWhat;

	fRecentDateWhat = _what;

	BMessenger messenger(kDeskbarSignature);
	messenger.SendMessage(what);
}


void
SettingsView::_UpdateTimeSettings(const uint32 what) const
{
	BMessenger messenger(kDeskbarSignature);
	messenger.SendMessage(what);
}
