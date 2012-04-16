/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 *		Alexander von Gluck, kallisti5@unixzen.com 
 */


#include "PowerStatusView.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Deskbar.h>
#include <Dragger.h>
#include <Drivers.h>
#include <File.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <TextView.h>

#include "ACPIDriverInterface.h"
#include "APMDriverInterface.h"
#include "ExtendedInfoWindow.h"
#include "PowerStatus.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PowerStatus"


extern "C" _EXPORT BView *instantiate_deskbar_item(void);
extern const char* kDeskbarItemName;

const uint32 kMsgToggleLabel = 'tglb';
const uint32 kMsgToggleTime = 'tgtm';
const uint32 kMsgToggleStatusIcon = 'tgsi';
const uint32 kMsgToggleExtInfo = 'texi';

const uint32 kMinIconWidth = 16;
const uint32 kMinIconHeight = 16;

PowerStatusView::PowerStatusView(PowerStatusDriverInterface* interface,
		BRect frame, int32 resizingMode,  int batteryID, bool inDeskbar)
	:
	BView(frame, kDeskbarItemName, resizingMode,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fDriverInterface(interface),
	fBatteryID(batteryID),
	fInDeskbar(inDeskbar)
{
	fPreferredSize.width = frame.Width();
	fPreferredSize.height = frame.Height();
	_Init();
}


PowerStatusView::PowerStatusView(BMessage* archive)
	: BView(archive)
{
	_Init();
	FromMessage(archive);
}


PowerStatusView::~PowerStatusView()
{
}


status_t
PowerStatusView::Archive(BMessage* archive, bool deep) const
{
	status_t status = BView::Archive(archive, deep);
	if (status == B_OK)
		status = ToMessage(archive);

	return status;
}


void
PowerStatusView::_Init()
{
	SetViewColor(B_TRANSPARENT_COLOR);

	fShowLabel = true;
	fShowTime = false;
	fShowStatusIcon = true;

	fPercent = -1;
	fOnline = true;
	fTimeLeft = 0;
}


void
PowerStatusView::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (Parent())
		SetLowColor(Parent()->ViewColor());
	else
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	Update();
}


void
PowerStatusView::DetachedFromWindow()
{
}


void
PowerStatusView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgUpdate:
			Update();
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
PowerStatusView::GetPreferredSize(float *width, float *height)
{
	*width = fPreferredSize.width;
	*height = fPreferredSize.height;
}


void
PowerStatusView::_DrawBattery(BRect rect)
{
	float quarter = floorf((rect.Height() + 1) / 4);
	rect.top += quarter;
	rect.bottom -= quarter;

	rect.InsetBy(2, 0);

	float left = rect.left;
	rect.left += rect.Width() / 11;

	SetHighColor(0, 0, 0);

	float gap = 1;
	if (rect.Height() > 8) {
		gap = ceilf((rect.left - left) / 2);

		// left
		FillRect(BRect(rect.left, rect.top, rect.left + gap - 1, rect.bottom));
		// right
		FillRect(BRect(rect.right - gap + 1, rect.top, rect.right,
			rect.bottom));
		// top
		FillRect(BRect(rect.left + gap, rect.top, rect.right - gap,
			rect.top + gap - 1));
		// bottom
		FillRect(BRect(rect.left + gap, rect.bottom + 1 - gap,
			rect.right - gap, rect.bottom));
	} else
		StrokeRect(rect);

	FillRect(BRect(left, floorf(rect.top + rect.Height() / 4) + 1,
		rect.left - 1, floorf(rect.bottom - rect.Height() / 4)));

	int32 percent = fPercent;
	if (percent > 100)
		percent = 100;
	else if (percent < 0 || !fHasBattery)
		percent = 0;

	if (percent > 0) {
		rect.InsetBy(gap, gap);
		rgb_color base = {84, 84, 84, 255};
		if (be_control_look != NULL) {
			BRect empty = rect;
			if (fHasBattery && percent > 0)
				empty.left += empty.Width() * percent / 100.0;

			be_control_look->DrawButtonBackground(this, empty, empty, base,
				fHasBattery
					? BControlLook::B_ACTIVATED : BControlLook::B_DISABLED,
				fHasBattery && percent > 0
					? (BControlLook::B_ALL_BORDERS
						& ~BControlLook::B_LEFT_BORDER)
					: BControlLook::B_ALL_BORDERS);
		}

		if (fHasBattery) {
			if (percent <= 15)
				base.set_to(180, 0, 0);
			else
				base.set_to(20, 180, 0);

			rect.right = rect.left + rect.Width() * percent / 100.0;

			if (be_control_look != NULL) {
				be_control_look->DrawButtonBackground(this, rect, rect, base,
					fHasBattery ? 0 : BControlLook::B_DISABLED);
			} else
				FillRect(rect);
		}
	}

	SetHighColor(0, 0, 0);
}


void
PowerStatusView::Draw(BRect updateRect)
{
	bool drawBackground = Parent() == NULL
		|| (Parent()->Flags() & B_DRAW_ON_CHILDREN) == 0;
	if (drawBackground)
		FillRect(updateRect, B_SOLID_LOW);

	float aspect = Bounds().Width() / Bounds().Height();
	bool below = aspect <= 1.0f;

	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float baseLine = ceilf(fontHeight.ascent);

	char text[64];
	_SetLabel(text, sizeof(text));

	float textHeight = ceilf(fontHeight.descent + fontHeight.ascent);
	float textWidth = StringWidth(text);
	bool showLabel = fShowLabel && text[0];

	BRect iconRect;

	if (fShowStatusIcon) {
		iconRect = Bounds();
		if (showLabel) {
			if (below)
				iconRect.bottom -= textHeight + 4;
			else
				iconRect.right -= textWidth + 4;
		}

		// make a square
		iconRect.bottom = min_c(iconRect.bottom, iconRect.right);
		iconRect.right = iconRect.bottom;

		if (iconRect.Width() + 1 >= kMinIconWidth
			&& iconRect.Height() + 1 >= kMinIconHeight) {
			_DrawBattery(iconRect);
		} else {
			// there is not enough space for the icon
			iconRect.Set(0, 0, -1, -1);
		}
	}

	if (showLabel) {
		BPoint point(0, baseLine);

		if (iconRect.IsValid()) {
			if (below) {
				point.x = (iconRect.Width() - textWidth) / 2;
				point.y += iconRect.Height() + 2;
			} else {
				point.x = iconRect.Width() + 2;
				point.y += (iconRect.Height() - textHeight) / 2;
			}
		} else {
			point.x = (Bounds().Width() - textWidth) / 2;
			point.y += (Bounds().Height() - textHeight) / 2;
		}

		if (drawBackground)
			SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));
		else {
			SetDrawingMode(B_OP_OVER);
			rgb_color c = Parent()->LowColor();
			if (c.red + c.green + c.blue > 128 * 3)
				SetHighColor(0, 0, 0);
			else
				SetHighColor(255, 255, 255);
		}

		DrawString(text, point);
	}
}


void
PowerStatusView::_SetLabel(char* buffer, size_t bufferLength)
{
	if (bufferLength < 1)
		return;

	buffer[0] = '\0';

	if (!fShowLabel)
		return;

	const char* open = "";
	const char* close = "";
	if (fOnline) {
		open = "(";
		close = ")";
	}

	if (!fShowTime && fPercent >= 0)
		snprintf(buffer, bufferLength, "%s%ld%%%s", open, fPercent, close);
	else if (fShowTime && fTimeLeft >= 0) {
		snprintf(buffer, bufferLength, "%s%ld:%02ld%s",
			open, fTimeLeft / 3600, (fTimeLeft / 60) % 60, close);
	}
}



void
PowerStatusView::Update(bool force)
{
	int32 previousPercent = fPercent;
	bool previousTimeLeft = fTimeLeft;
	bool wasOnline = fOnline;

	_GetBatteryInfo(&fBatteryInfo, fBatteryID);

	fHasBattery = (fBatteryInfo.state & BATTERY_CRITICAL_STATE) == 0;

	if (fBatteryInfo.full_capacity > 0 && fHasBattery) {
		fPercent = (100 * fBatteryInfo.capacity) / fBatteryInfo.full_capacity;
		fOnline = (fBatteryInfo.state & BATTERY_CHARGING) != 0;
		fTimeLeft = fBatteryInfo.time_left;
	} else {
		fPercent = 0;
		fOnline = false;
		fTimeLeft = false;
	}


	if (fInDeskbar) {
		// make sure the tray icon is large enough
		float width = fShowStatusIcon ? kMinIconWidth + 2 : 0;

		if (fShowLabel) {
			char text[64];
			_SetLabel(text, sizeof(text));

			if (text[0])
				width += ceilf(StringWidth(text)) + 4;
		} else {
			char text[256];
			const char* open = "";
			const char* close = "";
			if (fOnline) {
				open = "(";
				close = ")";
			}
			if (fHasBattery) {
				size_t length = snprintf(text, sizeof(text), "%s%ld%%%s",
					open, fPercent, close);
				if (fTimeLeft) {
					length += snprintf(text + length, sizeof(text) - length,
						"\n%ld:%02ld", fTimeLeft / 3600, (fTimeLeft / 60) % 60);
				}

				const char* state = NULL;
				if ((fBatteryInfo.state & BATTERY_CHARGING) != 0)
					state = B_TRANSLATE("charging");
				else if ((fBatteryInfo.state & BATTERY_DISCHARGING) != 0)
					state = B_TRANSLATE("discharging");

				if (state != NULL) {
					snprintf(text + length, sizeof(text) - length, "\n%s",
						state);
				}
			} else
				strcpy(text, B_TRANSLATE("no battery"));
			SetToolTip(text);
		}
		if (width == 0) {
			// make sure we're not going away completely
			width = 8;
		}

		if (width != Bounds().Width())
			ResizeTo(width, Bounds().Height());
	}

	if (force || wasOnline != fOnline
		|| (fShowTime && fTimeLeft != previousTimeLeft)
		|| (!fShowTime && fPercent != previousPercent))
		Invalidate();
}


void
PowerStatusView::FromMessage(const BMessage* archive)
{
	bool value;
	if (archive->FindBool("show label", &value) == B_OK)
		fShowLabel = value;
	if (archive->FindBool("show icon", &value) == B_OK)
		fShowStatusIcon = value;
	if (archive->FindBool("show time", &value) == B_OK)
		fShowTime = value;

	int32 intValue;
	if (archive->FindInt32("battery id", &intValue) == B_OK)
		fBatteryID = intValue;
}


status_t
PowerStatusView::ToMessage(BMessage* archive) const
{
	status_t status = archive->AddBool("show label", fShowLabel);
	if (status == B_OK)
		status = archive->AddBool("show icon", fShowStatusIcon);
	if (status == B_OK)
		status = archive->AddBool("show time", fShowTime);
	if (status == B_OK)
		status = archive->AddInt32("battery id", fBatteryID);

	return status;
}


void
PowerStatusView::_GetBatteryInfo(battery_info* batteryInfo, int batteryID)
{
	if (batteryID >= 0) {
		fDriverInterface->GetBatteryInfo(batteryInfo, batteryID);
	} else {
		for (int i = 0; i < fDriverInterface->GetBatteryCount(); i++) {
			battery_info info;
			fDriverInterface->GetBatteryInfo(&info, i);

			if (i == 0)
				*batteryInfo = info;
			else {
				batteryInfo->state &= info.state;
				batteryInfo->capacity += info.capacity;
				batteryInfo->full_capacity += info.full_capacity;
				batteryInfo->time_left += info.time_left;
			}
		}
	}
}


//	#pragma mark -


PowerStatusReplicant::PowerStatusReplicant(BRect frame, int32 resizingMode,
		bool inDeskbar)
	:
	PowerStatusView(NULL, frame, resizingMode, -1, inDeskbar)
{
	_Init();
	_LoadSettings();

	if (!inDeskbar) {
		// we were obviously added to a standard window - let's add a dragger
		frame.OffsetTo(B_ORIGIN);
		frame.top = frame.bottom - 7;
		frame.left = frame.right - 7;
		BDragger* dragger = new BDragger(frame, this,
			B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		AddChild(dragger);
	} else
		Update();
}


PowerStatusReplicant::PowerStatusReplicant(BMessage* archive)
	:
	PowerStatusView(archive)
{
	_Init();
	_LoadSettings();
}


PowerStatusReplicant::~PowerStatusReplicant()
{
	if (fMessengerExist) {
		delete fExtWindowMessenger;
	}

	fDriverInterface->StopWatching(this);
	fDriverInterface->Disconnect();
	fDriverInterface->ReleaseReference();

	_SaveSettings();
}


PowerStatusReplicant*
PowerStatusReplicant::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "PowerStatusReplicant"))
		return NULL;

	return new PowerStatusReplicant(archive);
}


status_t
PowerStatusReplicant::Archive(BMessage* archive, bool deep) const
{
	status_t status = PowerStatusView::Archive(archive, deep);
	if (status == B_OK)
		status = archive->AddString("add_on", kSignature);
	if (status == B_OK)
		status = archive->AddString("class", "PowerStatusReplicant");

	return status;
}


void
PowerStatusReplicant::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgToggleLabel:
			fShowLabel = !fShowLabel;
			Update(true);
			break;

		case kMsgToggleTime:
			fShowTime = !fShowTime;
			Update(true);
			break;

		case kMsgToggleStatusIcon:
			fShowStatusIcon = !fShowStatusIcon;
			Update(true);
			break;

		case kMsgToggleExtInfo:
			_OpenExtendedWindow();
			break;

		case B_ABOUT_REQUESTED:
			_AboutRequested();
			break;

		case B_QUIT_REQUESTED:
			_Quit();
			break;

		default:
			PowerStatusView::MessageReceived(message);
	}
}


void
PowerStatusReplicant::MouseDown(BPoint point)
{
	BPopUpMenu *menu = new BPopUpMenu(B_EMPTY_STRING, false, false);
	menu->SetFont(be_plain_font);

	BMenuItem* item;
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Show text label"),
		new BMessage(kMsgToggleLabel)));
	if (fShowLabel)
		item->SetMarked(true);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Show status icon"),
		new BMessage(kMsgToggleStatusIcon)));
	if (fShowStatusIcon)
		item->SetMarked(true);
	menu->AddItem(new BMenuItem(!fShowTime ? B_TRANSLATE("Show time") :
	B_TRANSLATE("Show percent"),
		new BMessage(kMsgToggleTime)));

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Battery info" B_UTF8_ELLIPSIS),
		new BMessage(kMsgToggleExtInfo)));

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS),
		new BMessage(B_ABOUT_REQUESTED)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), 
		new BMessage(B_QUIT_REQUESTED)));
	menu->SetTargetForItems(this);

	ConvertToScreen(&point);
	menu->Go(point, true, false, true);
}


void
PowerStatusReplicant::_AboutRequested()
{
	BAlert* alert = new BAlert(B_TRANSLATE("About"),
		B_TRANSLATE("PowerStatus\n"
			"written by Axel Dörfler, Clemens Zeidler\n"
			"Copyright 2006, Haiku, Inc.\n"), B_TRANSLATE("OK"));
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, strlen(B_TRANSLATE("PowerStatus")), &font);

	alert->Go();
}


void
PowerStatusReplicant::_Init()
{
	fDriverInterface = new ACPIDriverInterface;
	if (fDriverInterface->Connect() != B_OK) {
		delete fDriverInterface;
		fDriverInterface = new APMDriverInterface;
		if (fDriverInterface->Connect() != B_OK) {
			fprintf(stderr, "No power interface found.\n");
			_Quit();
		}
	}

	fExtendedWindow = NULL;
	fMessengerExist = false;
	fExtWindowMessenger = NULL;

	fDriverInterface->StartWatching(this);
}


void
PowerStatusReplicant::_Quit()
{
	if (fInDeskbar) {
		BDeskbar deskbar;
		deskbar.RemoveItem(kDeskbarItemName);
	} else
		be_app->PostMessage(B_QUIT_REQUESTED);
}


status_t
PowerStatusReplicant::_GetSettings(BFile& file, int mode)
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path,
		(mode & O_ACCMODE) != O_RDONLY);
	if (status != B_OK)
		return status;

	path.Append("PowerStatus settings");

	return file.SetTo(path.Path(), mode);
}


void
PowerStatusReplicant::_LoadSettings()
{
	fShowLabel = false;

	BFile file;
	if (_GetSettings(file, B_READ_ONLY) != B_OK)
		return;

	BMessage settings;
	if (settings.Unflatten(&file) < B_OK)
		return;

	FromMessage(&settings);
}


void
PowerStatusReplicant::_SaveSettings()
{
	BFile file;
	if (_GetSettings(file, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE) != B_OK)
		return;

	BMessage settings('pwst');
	ToMessage(&settings);

	ssize_t size = 0;
	settings.Flatten(&file, &size);
}


void
PowerStatusReplicant::_OpenExtendedWindow()
{
	if (!fExtendedWindow) {
		fExtendedWindow = new ExtendedInfoWindow(fDriverInterface);
		fExtWindowMessenger = new BMessenger(NULL, fExtendedWindow);
		fExtendedWindow->Show();
		return;
	}

	BMessage msg(B_SET_PROPERTY);
	msg.AddSpecifier("Hidden", int32(0));
	if (fExtWindowMessenger->SendMessage(&msg) == B_BAD_PORT_ID) {
		fExtendedWindow = new ExtendedInfoWindow(fDriverInterface);
		if (fMessengerExist)
			delete fExtWindowMessenger;
		fExtWindowMessenger = new BMessenger(NULL, fExtendedWindow);
		fMessengerExist = true;
		fExtendedWindow->Show();
	} else
		fExtendedWindow->Activate();

}


//	#pragma mark -


extern "C" _EXPORT BView*
instantiate_deskbar_item(void)
{
	return new PowerStatusReplicant(BRect(0, 0, 15, 15), B_FOLLOW_NONE, true);
}

