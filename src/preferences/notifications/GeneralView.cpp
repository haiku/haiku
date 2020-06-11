/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 *		Brian Hill, supernova@tycho.email
 */

#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Node.h>
#include <Path.h>
#include <Query.h>
#include <Roster.h>
#include <String.h>
#include <StringFormat.h>
#include <SymLink.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <notification/Notifications.h>

#include "GeneralView.h"
#include "NotificationsConstants.h"
#include "SettingsHost.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GeneralView"

const uint32 kToggleNotifications = '_TSR';
const uint32 kWidthChanged = '_WIC';
const uint32 kTimeoutChanged = '_TIC';
const uint32 kPositionChanged = '_NPC';
const uint32 kServerChangeTriggered = '_SCT';
const BString kSampleMessageID("NotificationsSample");


static int32
notification_position_to_index(uint32 notification_position) {
	if (notification_position == B_FOLLOW_NONE)
		return 0;
	else if (notification_position == (B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM))
		return 1;
	else if (notification_position == (B_FOLLOW_LEFT | B_FOLLOW_BOTTOM))
		return 2;
	else if (notification_position == (B_FOLLOW_RIGHT | B_FOLLOW_TOP))
		return 3;
	else if (notification_position == (B_FOLLOW_LEFT | B_FOLLOW_TOP))
		return 4;
	return 0;
}


GeneralView::GeneralView(SettingsHost* host)
	:
	SettingsPane("general", host)
{
	// Notification server
	fNotificationBox = new BCheckBox("server",
		B_TRANSLATE("Enable notifications"),
		new BMessage(kToggleNotifications));
	BBox* box = new BBox("box");
	box->SetLabel(fNotificationBox);

	// Window width
	float ratio = be_plain_font->Size() / 12.f;
	int32 minWidth = int32(kMinimumWidth / kWidthStep * ratio);
	int32 maxWidth = int32(kMaximumWidth / kWidthStep * ratio);
	fWidthSlider = new BSlider("width", B_TRANSLATE("Window width"),
		new BMessage(kWidthChanged), minWidth, maxWidth, B_HORIZONTAL);
	fWidthSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fWidthSlider->SetHashMarkCount(maxWidth - minWidth + 1);
	fWidthSlider->SetLimitLabels(
		B_TRANSLATE_COMMENT("narrow", "Window width: Slider low text"),
		B_TRANSLATE_COMMENT("wide", "Window width: Slider high text"));

	// Display time
	fDurationSlider = new BSlider("duration", B_TRANSLATE("Duration:"),
		new BMessage(kTimeoutChanged), kMinimumTimeout, kMaximumTimeout,
		B_HORIZONTAL);
	fDurationSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fDurationSlider->SetHashMarkCount(kMaximumTimeout - kMinimumTimeout + 1);
	BString minLabel;
	minLabel << kMinimumTimeout;
	BString maxLabel;
	maxLabel << kMaximumTimeout;
	fDurationSlider->SetLimitLabels(
		B_TRANSLATE_COMMENT(minLabel.String(), "Slider low text"),
		B_TRANSLATE_COMMENT(maxLabel.String(), "Slider high text"));

	// Notification Position
	fPositionMenu = new BPopUpMenu(B_TRANSLATE("Follow Deskbar"));
	const char* positionLabels[] = {
		B_TRANSLATE_MARK("Follow Deskbar"),
		B_TRANSLATE_MARK("Lower right"),
		B_TRANSLATE_MARK("Lower left"),
		B_TRANSLATE_MARK("Upper right"),
		B_TRANSLATE_MARK("Upper left")
	};
	const uint32 positions[] = {
		B_FOLLOW_DESKBAR,                   // Follow Deskbar
		B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT,   // Lower right
		B_FOLLOW_BOTTOM | B_FOLLOW_LEFT,    // Lower left
		B_FOLLOW_TOP    | B_FOLLOW_RIGHT,   // Upper right
		B_FOLLOW_TOP    | B_FOLLOW_LEFT     // Upper left
	};
	for (int i=0; i < 5; i++) {
		BMessage* message = new BMessage(kPositionChanged);
		message->AddInt32(kNotificationPositionName, positions[i]);

		fPositionMenu->AddItem(new BMenuItem(B_TRANSLATE_NOCOLLECT(
			positionLabels[i]), message));
	}
	BMenuField* positionField = new BMenuField(B_TRANSLATE("Position:"), 
		fPositionMenu);

	box->AddChild(BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fWidthSlider)
		.Add(fDurationSlider)
		.Add(positionField)
		.AddGlue()
		.View());
	
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(box)
	.End();
}


void
GeneralView::AttachedToWindow()
{
	BView::AttachedToWindow();
	fNotificationBox->SetTarget(this);
	fWidthSlider->SetTarget(this);
	fDurationSlider->SetTarget(this);
	fPositionMenu->SetTargetForItems(this);
}


void
GeneralView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kToggleNotifications:
		{
			SettingsPane::SettingsChanged(false);
			_EnableControls();
			break;
		}
		case kWidthChanged: {
			SettingsPane::SettingsChanged(true);
			break;
		}
		case kTimeoutChanged:
		{
			int32 value = fDurationSlider->Value();
			_SetTimeoutLabel(value);
			SettingsPane::SettingsChanged(true);
			break;
		}
		case kPositionChanged:
		{
			int32 position;
			if (msg->FindInt32(kNotificationPositionName, &position) == B_OK) {
				fNewPosition = position;
				SettingsPane::SettingsChanged(true);
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


status_t
GeneralView::Load(BMessage& settings)
{
	bool autoStart = settings.GetBool(kAutoStartName, true);
	fNotificationBox->SetValue(autoStart ? B_CONTROL_ON : B_CONTROL_OFF);

	if (settings.FindFloat(kWidthName, &fOriginalWidth) != B_OK
		|| fOriginalWidth > kMaximumWidth
		|| fOriginalWidth < kMinimumWidth)
		fOriginalWidth = kDefaultWidth;

	if (settings.FindInt32(kTimeoutName, &fOriginalTimeout) != B_OK
		|| fOriginalTimeout > kMaximumTimeout
		|| fOriginalTimeout < kMinimumTimeout)
		fOriginalTimeout = kDefaultTimeout;
// TODO need to save again if values outside of expected range
	int32 setting;
	if (settings.FindInt32(kIconSizeName, &setting) != B_OK)
		fOriginalIconSize = kDefaultIconSize;
	else
		fOriginalIconSize = (icon_size)setting;

	int32 position;
	if (settings.FindInt32(kNotificationPositionName, &position) != B_OK)
		fOriginalPosition = kDefaultNotificationPosition;
	else
		fOriginalPosition = position;

	_EnableControls();
	
	return Revert();
}


status_t
GeneralView::Save(BMessage& settings)
{
	bool autoStart = (fNotificationBox->Value() == B_CONTROL_ON);
	settings.AddBool(kAutoStartName, autoStart);

	int32 timeout = fDurationSlider->Value();
	settings.AddInt32(kTimeoutName, timeout);

	float width = fWidthSlider->Value() * kWidthStep;
	settings.AddFloat(kWidthName, width);

	icon_size iconSize = B_LARGE_ICON;
	settings.AddInt32(kIconSizeName, (int32)iconSize);

	settings.AddInt32(kNotificationPositionName, (int32)fNewPosition);

	return B_OK;
}


status_t
GeneralView::Revert()
{
	fDurationSlider->SetValue(fOriginalTimeout);
	_SetTimeoutLabel(fOriginalTimeout);
	
	fWidthSlider->SetValue(fOriginalWidth / kWidthStep);

	fNewPosition = fOriginalPosition;
	BMenuItem* item = fPositionMenu->ItemAt(
		notification_position_to_index(fNewPosition));
	if (item != NULL)
		item->SetMarked(true);
	
	return B_OK;
}


bool
GeneralView::RevertPossible()
{
	int32 timeout = fDurationSlider->Value();
	if (fOriginalTimeout != timeout)
		return true;
	
	int32 width = fWidthSlider->Value() * kWidthStep;
	if (fOriginalWidth != width)
		return true;

	if (fOriginalPosition != fNewPosition)
		return true;

	return false;
}


status_t
GeneralView::Defaults()
{
	fDurationSlider->SetValue(kDefaultTimeout);
	_SetTimeoutLabel(kDefaultTimeout);

	fWidthSlider->SetValue(kDefaultWidth / kWidthStep);

	fNewPosition = kDefaultNotificationPosition;
	BMenuItem* item = fPositionMenu->ItemAt(
		notification_position_to_index(fNewPosition));
	if (item != NULL)
		item->SetMarked(true);

	return B_OK;
}


bool
GeneralView::DefaultsPossible()
{
	int32 timeout = fDurationSlider->Value();
	if (kDefaultTimeout != timeout)
		return true;

	int32 width = fWidthSlider->Value() * kWidthStep;
	if (kDefaultWidth != width)
		return true;

	if (kDefaultNotificationPosition != fNewPosition)
		return true;
	
	return false;
}


bool
GeneralView::UseDefaultRevertButtons()
{
	return true;
}


void
GeneralView::_EnableControls()
{
	bool enabled = fNotificationBox->Value() == B_CONTROL_ON;
	fWidthSlider->SetEnabled(enabled);
	fDurationSlider->SetEnabled(enabled);
	BMenuItem* item = fPositionMenu->ItemAt(
		notification_position_to_index(fOriginalPosition));
	if (item != NULL)
		item->SetMarked(true);
}


void
GeneralView::_SetTimeoutLabel(int32 value)
{
	static BStringFormat format(B_TRANSLATE("{0, plural, "
		"=1{Timeout: # second}"
		"other{Timeout: # seconds}}"));
	BString label;
	format.Format(label, value);
	fDurationSlider->SetLabel(label.String());
}


bool
GeneralView::_IsServerRunning()
{
	return be_roster->IsRunning(kNotificationServerSignature);
}
