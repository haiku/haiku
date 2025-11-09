/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Nathan Patrizi, nathan.patrizi@gmail.com
 */


#include "LocationStringView.h"

#include <Cursor.h>
#include <Entry.h>
#include <Font.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <StringView.h>


LocationStringView::LocationStringView(const char* name, const char* text)
	: BStringView(name, text)
{
	fOriginalHighColor = HighColor();
	fStyledAsLink = false;
}


void
LocationStringView::CheckAndSetStyleForLink()
{
	_StyleAsLink(_IsFileLink());
}


void
LocationStringView::MouseDown(BPoint point)
{
	if (fStyledAsLink) {
		BMessenger tracker("application/x-vnd.Be-TRAK");
		BMessage message(B_REFS_RECEIVED);

		BEntry dirEntry(fFilePathParent.Path());
		entry_ref dirRef;
		dirEntry.GetRef(&dirRef);

		BEntry entry(fFilePath.Path());
		node_ref node;
		entry.GetNodeRef(&node);

		message.AddRef("refs", &dirRef);
		message.AddData("nodeRefToSelect", B_RAW_TYPE, &node, sizeof(node_ref));

		tracker.SendMessage(&message);
	}
}


void
LocationStringView::MouseMoved(BPoint point, uint32 transit, const BMessage* dragMessage)
{
	if (!fStyledAsLink)
		return;

	switch (transit) {
		case B_ENTERED_VIEW:
			_MouseOver();
			break;

		case B_EXITED_VIEW:
			_MouseAway();
			break;
	}
}


void
LocationStringView::_MouseAway()
{
	BCursor defaultCursor(B_CURSOR_ID_SYSTEM_DEFAULT);
	SetViewCursor(&defaultCursor);

	BFont font;
	GetFont(&font);
	font.SetFace(B_REGULAR_FACE);
	SetFont(&font);
}


void
LocationStringView::_MouseOver()
{
	BCursor linkCursor(B_CURSOR_ID_FOLLOW_LINK);
	SetViewCursor(&linkCursor);

	BFont font;
	GetFont(&font);
	font.SetFace(B_UNDERSCORE_FACE);
	SetFont(&font);
}


bool
LocationStringView::_IsFileLink()
{
	BString filePath(Text());

	return filePath.StartsWith("file:");
}


void
LocationStringView::_StripFileProtocol()
{
	BString filePath(Text());

	filePath.RemoveFirst("file:");

	if (filePath.StartsWith("///"))
		filePath.RemoveFirst("//");
	else if (filePath.StartsWith("//localhost/"))
		filePath.RemoveFirst("//localhost");

	fFilePath = filePath.String();
	fFilePath.GetParent(&fFilePathParent);

	SetText(fFilePathParent.Path());
	SetToolTip(fFilePathParent.Path());
}


void
LocationStringView::_StyleAsLink(bool enableLinkStyle)
{
	fStyledAsLink = enableLinkStyle;

	if (enableLinkStyle) {
		SetHighUIColor(B_LINK_TEXT_COLOR);
		_StripFileProtocol();
	} else {
		SetHighColor(fOriginalHighColor);
		_MouseAway();
	}
}
