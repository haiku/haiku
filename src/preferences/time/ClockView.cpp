/*
 * Copyright 2004-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione <jscipione@gmail.com>
 */

#include "ClockView.h"

#include <Alignment.h>
#include <Box.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Message.h>
#include <Messenger.h>
#include <RadioButton.h>
#include <SpaceLayoutItem.h>
#include <Window.h>

#include "TimeMessages.h"


static const char*	kDeskbarSignature		= "application/x-vnd.Be-TSKB";

static const float	kIndentSpacing
	= be_control_look->DefaultItemSpacing() * 2.3;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Time"


ClockView::ClockView(const char* name)
	:
	BView(name, 0),
	fCachedShowClock(B_CONTROL_ON),
	fCachedShowSeconds(B_CONTROL_OFF),
	fCachedShowDayOfWeek(B_CONTROL_OFF),
	fCachedShowTimeZone(B_CONTROL_OFF)
{
	fShowClock = new BCheckBox(B_TRANSLATE("Show clock in Deskbar"),
		new BMessage(kShowHideTime));
	fShowSeconds = new BCheckBox(B_TRANSLATE("Display time with seconds"),
		new BMessage(kShowSeconds));
	fShowDayOfWeek = new BCheckBox(B_TRANSLATE("Show day of week"),
		new BMessage(kShowDayOfWeek));
	fShowTimeZone = new BCheckBox(B_TRANSLATE("Show time zone"),
		new BMessage(kShowTimeZone));

	BView* view = BLayoutBuilder::Group<>(B_VERTICAL, 0)
		.Add(fShowSeconds)
		.Add(fShowDayOfWeek)
		.Add(fShowTimeZone)
		.AddGlue()
		.SetInsets(B_USE_DEFAULT_SPACING)
		.View();

	BBox* showClockBox = new BBox("show clock box");
	showClockBox->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));
	showClockBox->SetLabel(fShowClock);
	showClockBox->AddChild(view);

	BLayoutBuilder::Group<>(this)
		.AddGroup(B_VERTICAL, 0)
			.Add(showClockBox)
		.End()
		.SetInsets(B_USE_DEFAULT_SPACING);
}


ClockView::~ClockView()
{
}


void
ClockView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	fShowClock->SetTarget(this);
	fShowSeconds->SetTarget(this);
	fShowDayOfWeek->SetTarget(this);
	fShowTimeZone->SetTarget(this);

	// Disable these controls initially, they'll be enabled
	// when we get a response from Deskbar.
	fShowClock->SetEnabled(false);
	fShowSeconds->SetEnabled(false);
	fShowDayOfWeek->SetEnabled(false);
	fShowTimeZone->SetEnabled(false);

	// Ask Deskbar for current clock settings, it will reply
	// asynchronously in MesssageReceived() below.
	BMessenger* messenger = new BMessenger(kDeskbarSignature);
	BMessenger replyMessenger(this);
	BMessage* message = new BMessage(kGetClockSettings);
	messenger->SendMessage(message, replyMessenger);
}


void
ClockView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kGetClockSettings:
		{
			// Get current clock settings from Deskbar
			bool showClock;
			bool showSeconds;
			bool showDayOfWeek;
			bool showTimeZone;

			if (message->FindBool("showSeconds", &showSeconds) == B_OK) {
				fCachedShowSeconds = showSeconds
					? B_CONTROL_ON : B_CONTROL_OFF;
				fShowSeconds->SetValue(fCachedShowSeconds);
				fShowSeconds->SetEnabled(true);
			}

			if (message->FindBool("showDayOfWeek", &showDayOfWeek) == B_OK) {
				fCachedShowDayOfWeek = showDayOfWeek
					? B_CONTROL_ON : B_CONTROL_OFF;
				fShowDayOfWeek->SetValue(fCachedShowDayOfWeek);
				fShowDayOfWeek->SetEnabled(true);
			}

			if (message->FindBool("showTimeZone", &showTimeZone) == B_OK) {
				fCachedShowTimeZone = showTimeZone
					? B_CONTROL_ON : B_CONTROL_OFF;
				fShowTimeZone->SetValue(fCachedShowTimeZone);
				fShowTimeZone->SetEnabled(true);
			}

			// do this one last because it might disable the others
			if (message->FindBool("showClock", &showClock) == B_OK) {
				fCachedShowClock = showClock ? B_CONTROL_ON : B_CONTROL_OFF;
				fShowClock->SetValue(fCachedShowClock);
				fShowClock->SetEnabled(true);
				fShowSeconds->SetEnabled(showClock);
				fShowDayOfWeek->SetEnabled(showClock);
				fShowTimeZone->SetEnabled(showClock);
			}
			break;
		}

		case kShowHideTime:
		{
			bool showClock;
			if (message->FindBool("showClock", &showClock) == B_OK) {
				// message originated from Deskbar, handle special
				fShowClock->SetValue(showClock ? B_CONTROL_ON : B_CONTROL_OFF);
				fShowSeconds->SetEnabled(showClock);
				fShowDayOfWeek->SetEnabled(showClock);
				fShowTimeZone->SetEnabled(showClock);

				Window()->PostMessage(kMsgChange);
				break;
					// don't fall through
			}
			showClock = fShowClock->Value() == B_CONTROL_ON;
			fShowSeconds->SetEnabled(showClock);
			fShowDayOfWeek->SetEnabled(showClock);
			fShowTimeZone->SetEnabled(showClock);
		}
		// fall-through
		case kShowSeconds:
		case kShowDayOfWeek:
		case kShowTimeZone:
		{
			BMessenger* messenger = new BMessenger(kDeskbarSignature);
			messenger->SendMessage(message);

			Window()->PostMessage(kMsgChange);

			break;
		}

		case kMsgRevert:
			_Revert();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


bool
ClockView::CheckCanRevert()
{
	return fShowClock->Value() != fCachedShowClock
		|| fShowSeconds->Value() != fCachedShowSeconds
		|| fShowDayOfWeek->Value() != fCachedShowDayOfWeek
		|| fShowTimeZone->Value() != fCachedShowTimeZone;
}


void
ClockView::_Revert()
{
	if (fShowClock->Value() != fCachedShowClock) {
		fShowClock->SetValue(fCachedShowClock);
		fShowClock->Invoke();
	}

	if (fShowSeconds->Value() != fCachedShowSeconds) {
		fShowSeconds->SetValue(fCachedShowSeconds);
		fShowSeconds->Invoke();
	}

	if (fShowDayOfWeek->Value() != fCachedShowDayOfWeek) {
		fShowDayOfWeek->SetValue(fCachedShowDayOfWeek);
		fShowDayOfWeek->Invoke();
	}

	if (fShowTimeZone->Value() != fCachedShowTimeZone) {
		fShowTimeZone->SetValue(fCachedShowTimeZone);
		fShowTimeZone->Invoke();
	}
}
