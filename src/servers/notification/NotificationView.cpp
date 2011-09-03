/*
 * Copyright 2010-2011, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Davidson, slaad@bong.com.au
 *		Mikael Eiman, mikael@eiman.tv
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 */

#include <stdlib.h>

#include <Font.h>
#include <IconUtils.h>
#include <Messenger.h>
#include <Picture.h>
#include <PropertyInfo.h>
#include <Region.h>
#include <Resources.h>
#include <Roster.h>
#include <StatusBar.h>
#include <StringView.h>
#include <TranslationUtils.h>

#include "NotificationView.h"
#include "NotificationWindow.h"

const char* kSmallIconAttribute	= "BEOS:M:STD_ICON";
const char* kLargeIconAttribute	= "BEOS:L:STD_ICON";
const char* kIconAttribute		= "BEOS:ICON";

static const int kIconStripeWidth = 16;

property_info message_prop_list[] = {
	{ "type", {B_GET_PROPERTY, B_SET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0}, "get the notification type"},
	{ "app", {B_GET_PROPERTY, B_SET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0}, "get notification's app"},
	{ "title", {B_GET_PROPERTY, B_SET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0}, "get notification's title"},
	{ "content", {B_GET_PROPERTY, B_SET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0}, "get notification's contents"},
	{ "icon", {B_GET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0}, "get icon as an archived bitmap"},
	{ "progress", {B_GET_PROPERTY, B_SET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0}, "get the progress (between 0.0 and 1.0)"},
	{ NULL }
};


NotificationView::NotificationView(NotificationWindow* win,
	notification_type type, const char* app, const char* title, const char* text,
	BMessage* details)
	:
	BView(BRect(0, 0, win->ViewWidth(), 1), "NotificationView",
		B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE
		| B_FRAME_EVENTS),
	fParent(win),
	fType(type),
	fRunner(NULL),
	fProgress(0.0f),
	fMessageID(""),
	fDetails(details),
	fBitmap(NULL),
	fIsFirst(false),
	fIsLast(false)
{
	BMessage iconMsg;
	if (fDetails->FindMessage("icon", &iconMsg) == B_OK)
		fBitmap = new BBitmap(&iconMsg);

	if (!fBitmap)
		_LoadIcon();

	const char* messageID = NULL;
	if (fDetails->FindString("messageID", &messageID) == B_OK)
		fMessageID = messageID;

	if (fDetails->FindFloat("progress", &fProgress) != B_OK)
		fProgress = 0.0f;

	// Progress is between 0 and 1
	if (fProgress < 0.0f)
		fProgress = 0.0f;
	if (fProgress > 1.0f)
		fProgress = 1.0f;

	SetText(app, title, text);
	ResizeToPreferred();

	switch (type) {
		case B_IMPORTANT_NOTIFICATION:
			SetViewColor(255, 255, 255);
			SetLowColor(255, 255, 255);
			break;
		case B_ERROR_NOTIFICATION:
			SetViewColor(ui_color(B_FAILURE_COLOR));
			SetLowColor(ui_color(B_FAILURE_COLOR));
			break;
		case B_PROGRESS_NOTIFICATION:
		{
			BRect frame(kIconStripeWidth * 3, Bounds().bottom - 36,
				Bounds().right - kEdgePadding, Bounds().bottom - kEdgePadding);
			BStatusBar* progress = new BStatusBar(frame, "progress");
			progress->SetBarHeight(12.0f);
			progress->SetMaxValue(1.0f);
			progress->Update(fProgress);
			
			BString label = "";
			label << (int)(fProgress * 100) << " %";
			progress->SetTrailingText(label);
			
			AddChild(progress);
		}
		default:
			SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			break;
	}
}


NotificationView::~NotificationView()
{
	delete fRunner;
	delete fDetails;
	delete fBitmap;

	LineInfoList::iterator lIt;
	for (lIt = fLines.begin(); lIt != fLines.end(); lIt++)
		delete (*lIt);
}


void
NotificationView::AttachedToWindow()
{
	BMessage msg(kRemoveView);
	msg.AddPointer("view", this);
	bigtime_t timeout = -1;

	if (fDetails->FindInt64("timeout", &timeout) != B_OK)
		timeout = fParent->Timeout() * 1000000;

	if (timeout > 0)
		fRunner = new BMessageRunner(BMessenger(Parent()), &msg, timeout, 1);
}


void
NotificationView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_GET_PROPERTY:
		{
			BMessage specifier;
			const char* property;
			BMessage reply(B_REPLY);
			bool msgOkay = true;

			if (msg->FindMessage("specifiers", 0, &specifier) != B_OK)
				msgOkay = false;
			if (specifier.FindString("property", &property) != B_OK)
				msgOkay = false;

			if (msgOkay) {
				if (strcmp(property, "type") == 0)
					reply.AddInt32("result", fType);

				if (strcmp(property, "app") == 0)
					reply.AddString("result", fApp);

				if (strcmp(property, "title") == 0)
					reply.AddString("result", fTitle);

				if (strcmp(property, "content") == 0)
					reply.AddString("result", fText);

				if (strcmp(property, "progress") == 0)
					reply.AddFloat("result", fProgress);

				if ((strcmp(property, "icon") == 0) && fBitmap) {
					BMessage archive;
					if (fBitmap->Archive(&archive) == B_OK)
						reply.AddMessage("result", &archive);
				}

				reply.AddInt32("error", B_OK);
			} else {
				reply.what = B_MESSAGE_NOT_UNDERSTOOD;
				reply.AddInt32("error", B_ERROR);
			}

			msg->SendReply(&reply);
			break;
		}
		case B_SET_PROPERTY:
		{
			BMessage specifier;
			const char* property;
			BMessage reply(B_REPLY);
			bool msgOkay = true;

			if (msg->FindMessage("specifiers", 0, &specifier) != B_OK)
				msgOkay = false;
			if (specifier.FindString("property", &property) != B_OK)
				msgOkay = false;

			if (msgOkay) {
				if (strcmp(property, "app") == 0)
					msg->FindString("data", &fApp);

				if (strcmp(property, "title") == 0)
					msg->FindString("data", &fTitle);

				if (strcmp(property, "content") == 0)
					msg->FindString("data", &fText);

				if (strcmp(property, "icon") == 0) {
					BMessage archive;
					if (msg->FindMessage("data", &archive) == B_OK) {
						delete fBitmap;
						fBitmap = new BBitmap(&archive);
					}
				}

				SetText(Application(), Title(), Text());
				Invalidate();

				reply.AddInt32("error", B_OK);
			} else {
				reply.what = B_MESSAGE_NOT_UNDERSTOOD;
				reply.AddInt32("error", B_ERROR);
			}

			msg->SendReply(&reply);
			break;
		}
		case kRemoveView:
		{
			BMessage remove(kRemoveView);
			remove.AddPointer("view", this);
			BMessenger msgr(Window());
			msgr.SendMessage( &remove );
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}


void
NotificationView::GetPreferredSize(float* w, float* h)
{
	*w = fParent->ViewWidth();
	*h = fHeight;

	if (fType == B_PROGRESS_NOTIFICATION) {
		*h += 16 + kEdgePadding;
			// 16 is progress bar default size as stated in the BeBook
	}
}


void
NotificationView::Draw(BRect updateRect)
{
	BRect progRect;

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	// Icon size
	float iconSize = (float)fParent->IconSize();
	
	BRect stripeRect = Bounds();
	int32 iconLayoutScale = max_c(1, ((int32)be_plain_font->Size() + 15) / 16);
	stripeRect.right = kIconStripeWidth * iconLayoutScale;
	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	FillRect(stripeRect);
	
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	// Rectangle for icon and overlay icon
	BRect iconRect(0, 0, 0, 0);

	// Draw icon
	if (fBitmap) {
		LineInfo* appLine = fLines.back();
		font_height fh;
		appLine->font.GetHeight(&fh);

		float title_bottom = appLine->location.y + fh.descent;

		float ix = kEdgePadding;
		float iy = 0;
		if (fParent->Layout() == TitleAboveIcon)
			iy = title_bottom + kEdgePadding + (Bounds().Height() - title_bottom
				- kEdgePadding * 2 - iconSize) / 2;
		else
			iy = (Bounds().Height() - iconSize) / 2.0;

		if (fType == B_PROGRESS_NOTIFICATION)
			// Move icon up by half progress bar height if it's present
			iy -= (progRect.Height() + kEdgePadding) / 2.0;

		iconRect.Set(ix, iy, ix + iconSize - 1.0, iy + iconSize - 1.0);
		DrawBitmapAsync(fBitmap, fBitmap->Bounds(), iconRect);
	}

	// Draw content
	LineInfoList::iterator lIt;
	for (lIt = fLines.begin(); lIt != fLines.end(); lIt++) {
		LineInfo *l = (*lIt);

		SetFont(&l->font);
		DrawString(l->text.String(), l->text.Length(), l->location);
	}

	rgb_color detailCol = ui_color(B_CONTROL_BORDER_COLOR);
	detailCol = tint_color(detailCol, B_LIGHTEN_2_TINT);

	// Draw the close widget
	BRect closeRect = Bounds();
	closeRect.InsetBy(kEdgePadding, kEdgePadding);
	closeRect.left = closeRect.right - kCloseSize;
	closeRect.bottom = closeRect.top + kCloseSize;

	PushState();
		SetHighColor(detailCol);
		StrokeRoundRect(closeRect, kSmallPadding, kSmallPadding);
		BRect closeCross = closeRect.InsetByCopy(kSmallPadding, kSmallPadding);
		StrokeLine(closeCross.LeftTop(), closeCross.RightBottom());
		StrokeLine(closeCross.LeftBottom(), closeCross.RightTop());
	PopState();

	Sync();
}


void
NotificationView::MouseDown(BPoint point)
{
	int32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	switch (buttons) {
		case B_PRIMARY_MOUSE_BUTTON:
		{
			BRect closeRect = Bounds().InsetByCopy(2,2);
			closeRect.left = closeRect.right - kCloseSize;
			closeRect.bottom = closeRect.top + kCloseSize;

			if (!closeRect.Contains(point)) {
				entry_ref launchRef;
				BString launchString;
				BMessage argMsg(B_ARGV_RECEIVED);
				BMessage refMsg(B_REFS_RECEIVED);
				entry_ref appRef;
				bool useArgv = false;
				BList messages;
				entry_ref ref;

				if (fDetails->FindString("onClickApp", &launchString) == B_OK)
					if (be_roster->FindApp(launchString.String(), &appRef) == B_OK)
						useArgv = true;
				if (fDetails->FindRef("onClickFile", &launchRef) == B_OK) {
					if (be_roster->FindApp(&launchRef, &appRef) == B_OK)
						useArgv = true;
				}

				if (fDetails->FindRef("onClickRef", &ref) == B_OK) {
					for (int32 i = 0; fDetails->FindRef("onClickRef", i, &ref) == B_OK; i++)
						refMsg.AddRef("refs", &ref);

					messages.AddItem((void*)&refMsg);
				}

				if (useArgv) {
					type_code type;
					int32 argc = 0;
					BString arg;

					BPath p(&appRef);
					argMsg.AddString("argv", p.Path());

					fDetails->GetInfo("onClickArgv", &type, &argc);
					argMsg.AddInt32("argc", argc + 1);

					for (int32 i = 0; fDetails->FindString("onClickArgv", i, &arg) == B_OK; i++)
						argMsg.AddString("argv", arg);

					messages.AddItem((void*)&argMsg);
				}

				BMessage tmp;
				for (int32 i = 0; fDetails->FindMessage("onClickMsg", i, &tmp) == B_OK; i++)
					messages.AddItem((void*)&tmp);

				if (fDetails->FindString("onClickApp", &launchString) == B_OK)
					be_roster->Launch(launchString.String(), &messages);
				else
					be_roster->Launch(&launchRef, &messages);
			}

			// Remove the info view after a click
			BMessage remove_msg(kRemoveView);
			remove_msg.AddPointer("view", this);

			BMessenger msgr(Parent());
			msgr.SendMessage(&remove_msg);
			break;
		}
	}
}


void
NotificationView::FrameResized( float w, float /*h*/)
{
	SetText(Application(), Title(), Text());
}


BHandler*
NotificationView::ResolveSpecifier(BMessage* msg, int32 index, BMessage* spec, int32 form, const char* prop)
{
	BPropertyInfo prop_info(message_prop_list);
	if (prop_info.FindMatch(msg, index, spec, form, prop) >= 0) {
		msg->PopSpecifier();
		return this;
	}

	return BView::ResolveSpecifier(msg, index, spec, form, prop);
}


status_t
NotificationView::GetSupportedSuites(BMessage* msg)
{
	msg->AddString("suites", "suite/x-vnd.Haiku-notification_server");
	BPropertyInfo prop_info(message_prop_list);
	msg->AddFlat("messages", &prop_info);
	return BView::GetSupportedSuites(msg);
}


const char*
NotificationView::Application() const
{
	return fApp.Length() > 0 ? fApp.String() : NULL;
}


const char*
NotificationView::Title() const
{
	return fTitle.Length() > 0 ? fTitle.String() : NULL;
}


const char*
NotificationView::Text() const
{
	return fText.Length() > 0 ? fText.String() : NULL;
}


void
NotificationView::SetText(const char* app, const char* title, const char* text,
	float newMaxWidth)
{
	if (newMaxWidth < 0)
		newMaxWidth = Bounds().Width() - (kEdgePadding * 2);

	// Delete old lines
	LineInfoList::iterator lIt;
	for (lIt = fLines.begin(); lIt != fLines.end(); lIt++)
		delete (*lIt);
	fLines.clear();

	fApp = app;
	fTitle = title;
	fText = text;

	float iconRight = kEdgePadding + kEdgePadding;
	if (fBitmap != NULL)
		iconRight += fParent->IconSize();

	font_height fh;
	be_bold_font->GetHeight(&fh);
	float fontHeight = ceilf(fh.leading) + ceilf(fh.descent)
		+ ceilf(fh.ascent);
	float y = fontHeight;

	// Title
	LineInfo* titleLine = new LineInfo;
	titleLine->text = fTitle;
	titleLine->font = *be_bold_font;

	titleLine->location = BPoint(iconRight, y);

	fLines.push_front(titleLine);
	y += fontHeight;

	// Rest of text is rendered with be_plain_font.
	be_plain_font->GetHeight(&fh);
	fontHeight = ceilf(fh.leading) + ceilf(fh.descent)
		+ ceilf(fh.ascent);

	// Split text into chunks between certain characters and compose the lines.
	const char kSeparatorCharacters[] = " \n-\\/";
	BString textBuffer = fText;
	textBuffer.ReplaceAll("\t", "    ");
	const char* chunkStart = textBuffer.String();
	float maxWidth = newMaxWidth - kEdgePadding - iconRight;
	LineInfo* line = NULL;
	ssize_t length = textBuffer.Length();
	while (chunkStart - textBuffer.String() < length) {
		size_t chunkLength = strcspn(chunkStart, kSeparatorCharacters) + 1;

		// Start a new line if either we didn't start one before,
		// the current offset
		BString tempText;
		if (line != NULL)
			tempText.SetTo(line->text);
		tempText.Append(chunkStart, chunkLength);

		if (line == NULL || chunkStart[0] == '\n'
			|| StringWidth(tempText) > maxWidth) {
			line = new LineInfo;
			line->font = *be_plain_font;
			line->location = BPoint(iconRight + kEdgePadding, y);

			fLines.push_front(line);
			y += fontHeight;

			// Skip the eventual new-line character at the beginning of this
			// chunk.
			if (chunkStart[0] == '\n') {
				chunkStart++;
				chunkLength--;
			}
			// Skip more new-line characters and move the line further down.
			while (chunkStart[0] == '\n') {
				chunkStart++;
				chunkLength--;
				line->location.y += fontHeight;
				y += fontHeight;
			}
			// Strip space at beginning of a new line.
			while (chunkStart[0] == ' ') {
				chunkLength--;
				chunkStart++;
			}
		}

		if (chunkStart[0] == '\0')
			break;

		// Append the chunk to the current line, which was either a new
		// line or the one from the previous iteration.
		line->text.Append(chunkStart, chunkLength);

		chunkStart += chunkLength;
	}

	fHeight = y + (kEdgePadding * 2);

	// Make sure icon fits
	if (fBitmap != NULL) {
		float minHeight = 0;
		if (fParent->Layout() == TitleAboveIcon) {
			LineInfo* appLine = fLines.back();
			font_height fh;
			appLine->font.GetHeight(&fh);
			minHeight = appLine->location.y + fh.descent;
		}

		minHeight += fBitmap->Bounds().Height() + 2 * kEdgePadding;
		if (fHeight < minHeight)
			fHeight = minHeight;
	}

	BMessenger messenger(Parent());
	messenger.SendMessage(kResizeToFit);
}


bool
NotificationView::HasMessageID(const char* id)
{
	return fMessageID == id;
}


const char*
NotificationView::MessageID()
{
	return fMessageID.String();
}


void
NotificationView::SetPosition(bool first, bool last)
{
	fIsFirst = first;
	fIsLast = last;
}


BBitmap*
NotificationView::_ReadNodeIcon(const char* fileName, icon_size size)
{
	BEntry entry(fileName, true);

	entry_ref ref;
	entry.GetRef(&ref);

	BNode node(BPath(&ref).Path());

	BBitmap* ret = new BBitmap(BRect(0, 0, (float)size - 1, (float)size - 1), B_RGBA32);
	if (BIconUtils::GetIcon(&node, kIconAttribute, kSmallIconAttribute,
		kLargeIconAttribute, size, ret) != B_OK) {
		delete ret;
		ret = NULL;
	}

	return ret;
}


void
NotificationView::_LoadIcon()
{
	// First try to get the icon from the caller application
	app_info info;
	BMessenger msgr = fDetails->ReturnAddress();

	if (msgr.IsValid())
		be_roster->GetRunningAppInfo(msgr.Team(), &info);
	else if (fType == B_PROGRESS_NOTIFICATION)
		be_roster->GetAppInfo("application/x-vnd.Haiku-notification_server",
			&info);

	BPath path;
	path.SetTo(&info.ref);

	fBitmap = _ReadNodeIcon(path.Path(), fParent->IconSize());
	if (fBitmap)
		return;

	// If that failed get icons from app_server
	if (find_directory(B_BEOS_SERVERS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("app_server");

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return;

	BResources res(&file);
	if (res.InitCheck() != B_OK)
		return;

	// Which one should we choose?
	const char* iconName = "";
	switch (fType) {
		case B_INFORMATION_NOTIFICATION:
			iconName = "info";
			break;
		case B_ERROR_NOTIFICATION:
			iconName = "stop";
			break;
		case B_IMPORTANT_NOTIFICATION:
			iconName = "warn";
			break;
		default:
			return;
	}

	// Allocate the bitmap
	fBitmap = new BBitmap(BRect(0, 0, (float)B_LARGE_ICON - 1,
		(float)B_LARGE_ICON - 1), B_RGBA32);
	if (!fBitmap || fBitmap->InitCheck() != B_OK) {
		fBitmap = NULL;
		return;
	}

	// Load raw icon data
	size_t size = 0;
	const uint8* data = (const uint8*)res.LoadResource(B_VECTOR_ICON_TYPE,
		iconName, &size);
	if ((data == NULL
		|| BIconUtils::GetVectorIcon(data, size, fBitmap) != B_OK))
		fBitmap = NULL;
}
