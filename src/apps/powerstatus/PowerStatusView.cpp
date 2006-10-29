/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "PowerStatusView.h"
#include "PowerStatus.h"

#include <Alert.h>
#include <Application.h>
#include <Deskbar.h>
#include <Dragger.h>
#include <Drivers.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <PopUpMenu.h>
#include <TextView.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


extern "C" _EXPORT BView *instantiate_deskbar_item(void);


const uint32 kMsgUpdate = 'updt';
const uint32 kMsgToggleLabel = 'tglb';
const uint32 kMsgToggleTime = 'tgtm';
const uint32 kMsgToggleStatusIcon = 'tgsi';

const uint32 kMinIconWidth = 16;
const uint32 kMinIconHeight = 16;

const bigtime_t kUpdateInterval = 1000000;
		// every second


#ifndef HAIKU_TARGET_PLATFORM_HAIKU
// definitions for the APM driver available for BeOS
enum {
	APM_CONTROL = B_DEVICE_OP_CODES_END + 1,
	APM_DUMP_POWER_STATUS,
	APM_BIOS_CALL,
	APM_SET_SAFETY
};

#define BIOS_APM_GET_POWER_STATUS 0x530a
#endif


PowerStatusView::PowerStatusView(BRect frame, int32 resizingMode, bool inDeskbar)
	: BView(frame, kDeskbarItemName, resizingMode,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fInDeskbar(inDeskbar)
{
	_Init();

	if (!inDeskbar) {
		// we were obviously added to a standard window - let's add a dragger
		frame.OffsetTo(B_ORIGIN);
		frame.top = frame.bottom - 7;
		frame.left = frame.right - 7;
		BDragger* dragger = new BDragger(frame, this,
			B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		AddChild(dragger);
	} else
		_Update();
}


PowerStatusView::PowerStatusView(BMessage* archive)
	: BView(archive)
{
	_Init();

	bool value;
	if (archive->FindBool("show label", &value) == B_OK)
		fShowLabel = value;
	if (archive->FindBool("show icon", &value) == B_OK)
		fShowStatusIcon = value;
	if (archive->FindBool("show time", &value) == B_OK)
		fShowTime = value;
}


PowerStatusView::~PowerStatusView()
{
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
	close(fDevice);
#endif
}


void
PowerStatusView::_Init()
{
	fShowLabel = true;
	fShowTime = false;
	fShowStatusIcon = true;

	fMessageRunner = NULL;
	fPercent = -1;
	fOnline = true;
	fTimeLeft = 0;

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	// TODO: implement me
#else
	fDevice = open("/dev/misc/apm", O_RDONLY);
	if (fDevice < 0) {
		fprintf(stderr, "No power interface found.\n");
		_Quit();
	}
#endif
}


void
PowerStatusView::_Quit()
{
	if (fInDeskbar) {
		BDeskbar deskbar;
		deskbar.RemoveItem(kDeskbarItemName);
	} else
		be_app->PostMessage(B_QUIT_REQUESTED);
}


PowerStatusView *
PowerStatusView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "PowerStatusView"))
		return NULL;

	return new PowerStatusView(archive);
}


status_t
PowerStatusView::Archive(BMessage* archive, bool deep) const
{
	status_t status = BView::Archive(archive, deep);
	if (status == B_OK)
		status = archive->AddString("add_on", kSignature);
	if (status == B_OK)
		status = archive->AddString("class", "PowerStatusView");
	if (status == B_OK)
		status = archive->AddBool("show label", fShowLabel);
	if (status == B_OK)
		status = archive->AddBool("show icon", fShowStatusIcon);
	if (status == B_OK)
		status = archive->AddBool("show time", fShowTime);

	return status;
}


void
PowerStatusView::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (Parent())
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	SetLowColor(ViewColor());

	BMessage update(kMsgUpdate);
	fMessageRunner = new BMessageRunner(this, &update, kUpdateInterval);

	_Update();
}


void
PowerStatusView::DetachedFromWindow()
{
	delete fMessageRunner;
}


void
PowerStatusView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgUpdate:
			_Update();
			break;

		case kMsgToggleLabel:
			fShowLabel = !fShowLabel;
			_Update(true);
			break;

		case kMsgToggleTime:
			fShowTime = !fShowTime;
			_Update(true);
			break;

		case kMsgToggleStatusIcon:
			fShowStatusIcon = !fShowStatusIcon;
			_Update(true);
			break;

		case B_ABOUT_REQUESTED:
			_AboutRequested();
			break;

		case B_QUIT_REQUESTED:
			_Quit();
			break;

		default:
			BView::MessageReceived(message);
	}
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

	float gap = 1;
	if (rect.Height() > 8) {
		rect.InsetBy(1, 1);
		SetPenSize(2);
		gap = 2;
	}

	if (fOnline)
		SetHighColor(92, 92, 92);

	StrokeRect(rect);

	SetPenSize(1);
	FillRect(BRect(left, floorf(rect.top + rect.Height() / 4) + 1,
		rect.left, floorf(rect.bottom - rect.Height() / 4)));

	int32 percent = fPercent;
	if (percent > 100 || percent < 0)
		percent = 100;

	if (percent > 0) {
		if (percent < 16)
			SetHighColor(180, 0, 0);

		rect.InsetBy(gap + 1, gap + 1);
		if (gap > 1) {
			rect.right++;
			rect.bottom++;
		}

		rect.right = rect.left + rect.Width() * min_c(percent, 100) / 100.0;
		FillRect(rect);
	}

	SetHighColor(0, 0, 0);
}


void
PowerStatusView::Draw(BRect updateRect)
{
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
			// TODO: have real icons
			//if (!fOnline)
				_DrawBattery(iconRect);
			//else
			//	FillRect(iconRect);
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

		DrawString(text, point);
	}
}


void
PowerStatusView::MouseDown(BPoint point)
{
	BPopUpMenu *menu = new BPopUpMenu(B_EMPTY_STRING, false, false);
	menu->SetFont(be_plain_font);

	BMenuItem* item;
	menu->AddItem(item = new BMenuItem("Show Text Label", new BMessage(kMsgToggleLabel)));
	if (fShowLabel)
		item->SetMarked(true);
	menu->AddItem(item = new BMenuItem("Show Status Icon",
		new BMessage(kMsgToggleStatusIcon)));
	if (fShowStatusIcon)
		item->SetMarked(true);
	menu->AddItem(new BMenuItem(!fShowTime ? "Show Time" : "Show Percent",
		new BMessage(kMsgToggleTime)));

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("About" B_UTF8_ELLIPSIS, new BMessage(B_ABOUT_REQUESTED)));
	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED)));
	menu->SetTargetForItems(this);

	ConvertToScreen(&point);
	menu->Go(point, true, false, true);
}


void
PowerStatusView::_AboutRequested()
{
	BAlert *alert = new BAlert("about", "PowerStatus\n"
		"\twritten by Axel Dörfler\n"
		"\tCopyright 2006, Haiku, Inc.\n", "Ok");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 11, &font);

	alert->Go();
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
		snprintf(buffer, bufferLength, "%s%ld:%ld%s",
			open, fTimeLeft / 3600, fTimeLeft / 60, close);
	}
}


void
PowerStatusView::_Update(bool force)
{
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	// TODO: retrieve data from APM/ACPI kernel interface
	fPercent = 42;
	fTimeLeft = 1500;
	fOnline = true;
#else
	if (fDevice < 0)
		return;

	uint16 regs[6] = {0, 0, 0, 0, 0, 0};
	regs[0] = BIOS_APM_GET_POWER_STATUS;
	regs[1] = 0x1;
	if (ioctl(fDevice, APM_BIOS_CALL, regs) == 0) {
		fOnline = (regs[1] >> 8) != 0 && (regs[1] >> 8) != 2;
		fPercent = regs[2] & 255;
		if (fPercent > 100)
			fPercent = -1;
		fTimeLeft = fPercent >= 0 ? regs[3] : -1;
		if (fTimeLeft > 0xffff)
			fTimeLeft = -1;
	}
#endif

	if (fInDeskbar) {
		// make sure the tray icon is large enough
		float width = fShowStatusIcon ? kMinIconWidth + 2 : 0;

		if (fShowLabel) {
			char text[64];
			_SetLabel(text, sizeof(text));

			if (text[0])
				width += ceilf(StringWidth(text)) + 4;
		}
		if (width == 0) {
			// make sure we're not going away completely
			width = 8;
		}

		if (width != Bounds().Width())
			ResizeTo(width, Bounds().Height());
	}

	if (force)
		Invalidate();
}


//	#pragma mark -


extern "C" _EXPORT BView *
instantiate_deskbar_item(void)
{
	return new PowerStatusView(BRect(0, 0, 15, 15), B_FOLLOW_NONE, true);
}

