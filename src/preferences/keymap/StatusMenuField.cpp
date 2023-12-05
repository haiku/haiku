/*
 * Copyright 2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include "StatusMenuField.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <IconUtils.h>
#include <InterfaceDefs.h>
#include <LayoutUtils.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Modifier keys window"


#ifdef DEBUG_ALERT
#	define FTRACE(x) fprintf(x)
#else
#	define FTRACE(x) /* nothing */
#endif


static const char* kDuplicate = "duplicate";
static const char* kUnmatched = "unmatched";


//	#pragma mark - StatusMenuItem


StatusMenuItem::StatusMenuItem(const char* name, BMessage* message)
	:
	BMenuItem(name, message),
	fIcon(NULL)
{
}


StatusMenuItem::StatusMenuItem(BMessage* archive)
	:
	BMenuItem(archive),
	fIcon(NULL)
{
}


BArchivable*
StatusMenuItem::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "StatusMenuItem"))
		return new StatusMenuItem(data);

	return NULL;
}


status_t
StatusMenuItem::Archive(BMessage* data, bool deep) const
{
	status_t result = BMenuItem::Archive(data, deep);

	return result;
}


void
StatusMenuItem::DrawContent()
{
	if (fIcon == NULL)
		return BMenuItem::DrawContent();

	// blend transparency
	Menu()->SetDrawingMode(B_OP_ALPHA);
	Menu()->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);

	// draw bitmap
	Menu()->DrawBitmapAsync(fIcon, IconRect().LeftTop());

	BMenuItem::DrawContent();
}


void
StatusMenuItem::GetContentSize(float* _width, float* _height)
{
	float width;
	float height;
	BMenuItem::GetContentSize(&width, &height);

	// prevent label from drawing over icon
	if (_width != NULL)
		*_width = Menu()->Bounds().Width() - IconRect().Width() - Spacing() * 4;

	if (_height != NULL)
		*_height = height;
}


BBitmap*
StatusMenuItem::Icon()
{
	return fIcon;
}


void
StatusMenuItem::SetIcon(BBitmap* icon)
{
	fIcon = icon;
}


BRect
StatusMenuItem::IconRect()
{
	BRect bounds(Menu()->Bounds());
	bounds.top += roundf(Spacing() / 2); // center inside menu field vertically
	bounds.right -= Spacing() * 3; // move inside menu field horizontally
	return BLayoutUtils::AlignInFrame(bounds, IconSize(),
		BAlignment(B_ALIGN_RIGHT, B_ALIGN_TOP));
}


BSize
StatusMenuItem::IconSize()
{
	return be_control_look->ComposeIconSize(B_MINI_ICON);
}


float
StatusMenuItem::Spacing()
{
	return be_control_look->DefaultLabelSpacing();
}


//	#pragma mark - StatusMenuField


StatusMenuField::StatusMenuField(const char* label, BMenu* menu)
	:
	BMenuField(label, menu),
	fStatus(B_EMPTY_STRING),
	fStopIcon(NULL),
	fWarnIcon(NULL)
{
	_FillIcons();
}


StatusMenuField::~StatusMenuField()
{
	delete fStopIcon;
	delete fWarnIcon;
}


void
StatusMenuField::SetDuplicate(bool on)
{
	ShowStopIcon(on);
	on ? SetStatus(kDuplicate) : ClearStatus();
	Invalidate();
}


void
StatusMenuField::SetUnmatched(bool on)
{
	ShowWarnIcon(on);
	on ? SetStatus(kUnmatched) : ClearStatus();
	Invalidate();
}


void
StatusMenuField::ShowStopIcon(bool show)
{
	// show or hide the stop icon
	StatusMenuItem* item = dynamic_cast<StatusMenuItem*>(MenuItem());
	if (item != NULL)
		item->SetIcon(show ? fStopIcon : NULL);
}


void
StatusMenuField::ShowWarnIcon(bool show)
{
	// show or hide the warn icon
	StatusMenuItem* item = dynamic_cast<StatusMenuItem*>(MenuItem());
	if (item != NULL)
		item->SetIcon(show ? fWarnIcon : NULL);
}


void
StatusMenuField::ClearStatus()
{
	fStatus = B_EMPTY_STRING;
	SetToolTip((const char*)NULL);
}


void
StatusMenuField::SetStatus(BString status)
{
	fStatus = status;

	const char* tooltip = B_EMPTY_STRING;
	if (fStatus == kDuplicate)
		tooltip = B_TRANSLATE("Error: duplicate keys");
	else if (fStatus == kUnmatched)
		tooltip = B_TRANSLATE("Warning: left and right key roles do not match");

	SetToolTip(tooltip);
}


//	#pragma mark - StatusMenuField private methods


void
StatusMenuField::_FillIcons()
{
	// fill out the icons with the stop and warn icons from app_server
	// TODO find better icons

	if (fStopIcon == NULL) {
		// allocate the fStopIcon bitmap
		fStopIcon = new (std::nothrow) BBitmap(_IconRect(), 0, B_RGBA32);
		if (fStopIcon == NULL || fStopIcon->InitCheck() != B_OK) {
			FTRACE((stderr, "MKW::_FillIcons() - No memory for stop bitmap\n"));
			delete fStopIcon;
			fStopIcon = NULL;
			return;
		}

		// load dialog-error icon bitmap
		if (BIconUtils::GetSystemIcon("dialog-error", fStopIcon) != B_OK) {
			delete fStopIcon;
			fStopIcon = NULL;
			return;
		}
	}

	if (fWarnIcon == NULL) {
		// allocate the fWarnIcon bitmap
		fWarnIcon = new (std::nothrow) BBitmap(_IconRect(), 0, B_RGBA32);
		if (fWarnIcon == NULL || fWarnIcon->InitCheck() != B_OK) {
			FTRACE((stderr, "MKW::_FillIcons() - No memory for warn bitmap\n"));
			delete fWarnIcon;
			fWarnIcon = NULL;
			return;
		}

		// load dialog-warning icon bitmap
		if (BIconUtils::GetSystemIcon("dialog-warning", fWarnIcon) != B_OK) {
			delete fWarnIcon;
			fWarnIcon = NULL;
			return;
		}
	}
}


BRect
StatusMenuField::_IconRect()
{
	BSize iconSize = be_control_look->ComposeIconSize(B_MINI_ICON);
	return BRect(0, 0, iconSize.Width() - 1, iconSize.Height() - 1);
}
