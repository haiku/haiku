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

#include <ControlLook.h>
#include <LayoutUtils.h>
#include <Messenger.h>
#include <Path.h>
#include <Roster.h>
#include <StatusBar.h>

#include "NotificationView.h"
#include "NotificationWindow.h"

static const int kIconStripeWidth = 32;

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
	BNotification* notification, bigtime_t timeout)
	:
	BView("NotificationView", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE
		| B_FRAME_EVENTS),
	fParent(win),
	fNotification(notification),
	fTimeout(timeout),
	fRunner(NULL),
	fBitmap(NULL)
{
	if (fNotification->Icon() != NULL)
		fBitmap = new BBitmap(fNotification->Icon());

	if (fTimeout <= 0)
		fTimeout = fParent->Timeout() * 1000000;

	SetText();

	switch (fNotification->Type()) {
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
			BRect frame(kIconStripeWidth + 9, Bounds().bottom - 36,
				Bounds().right - 8, Bounds().bottom - 10);
			BStatusBar* progress = new BStatusBar(frame, "progress");
			progress->SetBarHeight(12.0f);
			progress->SetMaxValue(1.0f);
			progress->Update(fNotification->Progress());
			
			BString label = "";
			label << (int)(fNotification->Progress() * 100) << " %";
			progress->SetTrailingText(label);
			
			AddChild(progress);
		}
		// fall through
		default:
			SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}
}


NotificationView::~NotificationView()
{
	delete fRunner;
	delete fBitmap;
	delete fNotification;

	LineInfoList::iterator lIt;
	for (lIt = fLines.begin(); lIt != fLines.end(); lIt++)
		delete (*lIt);
}


void
NotificationView::AttachedToWindow()
{
	BMessage msg(kRemoveView);
	msg.AddPointer("view", this);

	fRunner = new BMessageRunner(BMessenger(Parent()), &msg, fTimeout, 1);
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
					reply.AddInt32("result", fNotification->Type());

				if (strcmp(property, "group") == 0)
					reply.AddString("result", fNotification->Group());

				if (strcmp(property, "title") == 0)
					reply.AddString("result", fNotification->Title());

				if (strcmp(property, "content") == 0)
					reply.AddString("result", fNotification->Content());

				if (strcmp(property, "progress") == 0)
					reply.AddFloat("result", fNotification->Progress());

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
				const char* value = NULL;

				if (strcmp(property, "group") == 0)
					if (msg->FindString("data", &value) == B_OK)
						fNotification->SetGroup(value);

				if (strcmp(property, "title") == 0)
					if (msg->FindString("data", &value) == B_OK)
						fNotification->SetTitle(value);

				if (strcmp(property, "content") == 0)
					if (msg->FindString("data", &value) == B_OK)
						fNotification->SetContent(value);

				if (strcmp(property, "icon") == 0) {
					BMessage archive;
					if (msg->FindMessage("data", &archive) == B_OK) {
						delete fBitmap;
						fBitmap = new BBitmap(&archive);
					}
				}

				SetText();
				Invalidate();

				reply.AddInt32("error", B_OK);
			} else {
				reply.what = B_MESSAGE_NOT_UNDERSTOOD;
				reply.AddInt32("error", B_ERROR);
			}

			msg->SendReply(&reply);
			break;
		}
		default:
			BView::MessageReceived(msg);
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
	stripeRect.right = kIconStripeWidth;
	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	FillRect(stripeRect);
	
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	// Rectangle for icon and overlay icon
	BRect iconRect(0, 0, 0, 0);

	// Draw icon
	if (fBitmap) {
		float ix = 18;
		float iy = (Bounds().Height() - iconSize) / 4.0;
			// Icon is vertically centered in view

		if (fNotification->Type() == B_PROGRESS_NOTIFICATION)
		{
			// Move icon up by half progress bar height if it's present
			iy -= (progRect.Height() + kEdgePadding);
		}

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
	closeRect.InsetBy(2 * kEdgePadding, 2 * kEdgePadding);
	closeRect.left = closeRect.right - kCloseSize;
	closeRect.bottom = closeRect.top + kCloseSize;

	PushState();
		SetHighColor(detailCol);
		StrokeRoundRect(closeRect, kSmallPadding, kSmallPadding);
		BRect closeCross = closeRect.InsetByCopy(kSmallPadding, kSmallPadding);
		StrokeLine(closeCross.LeftTop(), closeCross.RightBottom());
		StrokeLine(closeCross.LeftBottom(), closeCross.RightTop());
	PopState();

	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	BPoint left(Bounds().left, Bounds().bottom - 1);
	BPoint right(Bounds().right, Bounds().bottom - 1);
	StrokeLine(left, right);

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

				if (fNotification->OnClickApp() != NULL
					&& be_roster->FindApp(fNotification->OnClickApp(), &appRef)
				   		== B_OK) {
					useArgv = true;
				}

				if (fNotification->OnClickFile() != NULL
					&& be_roster->FindApp(
							(entry_ref*)fNotification->OnClickFile(), &appRef)
				   		== B_OK) {
					useArgv = true;
				}

				for (int32 i = 0; i < fNotification->CountOnClickRefs(); i++)
					refMsg.AddRef("refs", fNotification->OnClickRefAt(i));
				messages.AddItem((void*)&refMsg);

				if (useArgv) {
					int32 argc = fNotification->CountOnClickArgs() + 1;
					BString arg;

					BPath p(&appRef);
					argMsg.AddString("argv", p.Path());

					argMsg.AddInt32("argc", argc);

					for (int32 i = 0; i < argc - 1; i++) {
						argMsg.AddString("argv",
							fNotification->OnClickArgAt(i));
					}

					messages.AddItem((void*)&argMsg);
				}

				if (fNotification->OnClickApp() != NULL)
					be_roster->Launch(fNotification->OnClickApp(), &messages);
				else
					be_roster->Launch(fNotification->OnClickFile(), &messages);
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


BSize
NotificationView::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), _CalculateSize());
}


BSize
NotificationView::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), _CalculateSize());
}


BSize
NotificationView::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		_CalculateSize());
}


BHandler*
NotificationView::ResolveSpecifier(BMessage* msg, int32 index, BMessage* spec,
	int32 form, const char* prop)
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


void
NotificationView::SetText(float newMaxWidth)
{
	if (newMaxWidth < 0)
		newMaxWidth = Bounds().Width() - (kEdgePadding * 2);

	// Delete old lines
	LineInfoList::iterator lIt;
	for (lIt = fLines.begin(); lIt != fLines.end(); lIt++)
		delete (*lIt);
	fLines.clear();

	float iconRight = kIconStripeWidth;
	if (fBitmap != NULL)
		iconRight += fParent->IconSize();
	else
		iconRight += 32;

	font_height fh;
	be_bold_font->GetHeight(&fh);
	float fontHeight = ceilf(fh.leading) + ceilf(fh.descent)
		+ ceilf(fh.ascent);
	float y = 2 * fontHeight;

	// Title
	LineInfo* titleLine = new LineInfo;
	titleLine->text = fNotification->Title();
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
	BString textBuffer = fNotification->Content();
	textBuffer.ReplaceAll("\t", "    ");
	const char* chunkStart = textBuffer.String();
	float maxWidth = newMaxWidth - kEdgePadding - iconRight;
	LineInfo* line = NULL;
	ssize_t length = textBuffer.Length();
	while (chunkStart - textBuffer.String() < length) {
		size_t chunkLength = strcspn(chunkStart, kSeparatorCharacters) + 1;

		// Start a new line if we didn't start one before
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

			// Skip the eventual new-line character at the beginning of this chunk
			if (chunkStart[0] == '\n') {
				chunkStart++;
				chunkLength--;
			}

			// Skip more new-line characters and move the line further down
			while (chunkStart[0] == '\n') {
				chunkStart++;
				chunkLength--;
				line->location.y += fontHeight;
				y += fontHeight;
			}

			// Strip space at beginning of a new line
			while (chunkStart[0] == ' ') {
				chunkLength--;
				chunkStart++;
			}
		}

		if (chunkStart[0] == '\0')
			break;

		// Append the chunk to the current line, which was either a new
		// line or the one from the previous iteration
		line->text.Append(chunkStart, chunkLength);

		chunkStart += chunkLength;
	}

	fHeight = y + (kEdgePadding * 2);

	// Make sure icon fits
	if (fBitmap != NULL) {
		float minHeight = fBitmap->Bounds().Height() + 2 * kEdgePadding;

		if (fHeight < minHeight)
			fHeight = minHeight;
	}
}


const char*
NotificationView::MessageID() const
{
	return fNotification->MessageID();
}


BSize
NotificationView::_CalculateSize()
{
	BSize size;

	// Parent width, minus the edge padding, minus the pensize
	size.width = fParent->Width() - (kEdgePadding * 2) - (kPenSize * 2);
	size.height = fHeight;

	if (fNotification->Type() == B_PROGRESS_NOTIFICATION) {
		font_height fh;
		be_plain_font->GetHeight(&fh);
		float fontHeight = fh.ascent + fh.descent + fh.leading;
		size.height += (kSmallPadding * 2) + (kEdgePadding * 1) + fontHeight;
	}

	return size;
}
