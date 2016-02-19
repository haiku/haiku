/*
 * Copyright 2005-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */


#include "AttributeTextControl.h"

#include <string.h>
#include <malloc.h>

#include <Catalog.h>
#include <Cursor.h>
#include <Font.h>
#include <Roster.h>
#include <StorageKit.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "People"

#define B_URL_MIME "application/x-vnd.Be.URL."

AttributeTextControl::AttributeTextControl(const char* label,
	const char* attribute)
	:
	BTextControl(NULL, "", NULL),
	fAttribute(attribute),
	fOriginalValue()
{
	if (label != NULL && label[0] != 0) {
		SetLabel(BString(B_TRANSLATE("%attribute_label:"))
			.ReplaceFirst("%attribute_label", label));
	}
	SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
}


AttributeTextControl::~AttributeTextControl()
{
}

void
AttributeTextControl::MouseDown(BPoint mousePosition)
{
	if(_VisibleLabelBounds().Contains(mousePosition) && _ContainsUrl())
		_HandleLabelClicked(Text());
	else
		BTextControl::MouseDown(mousePosition);
}


void
AttributeTextControl::MouseMoved(BPoint mousePosition, uint32 transit, const BMessage* dragMessage)
{
	if (_VisibleLabelBounds().Contains(mousePosition) && _ContainsUrl()) {
		BCursor linkCursor(B_CURSOR_ID_FOLLOW_LINK);
		SetViewCursor(&linkCursor, true);
	} else
		SetViewCursor(B_CURSOR_SYSTEM_DEFAULT, true);

	BTextControl::MouseMoved(mousePosition, transit, dragMessage);
}


bool
AttributeTextControl::HasChanged()
{
	return fOriginalValue != Text();
}


void
AttributeTextControl::Revert()
{
	if (HasChanged())
		SetText(fOriginalValue);
}


void
AttributeTextControl::Update()
{
	fOriginalValue = Text();
}


void
AttributeTextControl::_HandleLabelClicked(const char *text)
{
	BString argument(text);

	if (Attribute() == "META:url") {
		_MakeUniformUrl(argument);

		BString mimeType = B_URL_MIME;
		_BuildMimeString(mimeType, argument);

		if (mimeType != "") {
			const char *args[] = {argument.String(), 0};
			be_roster->Launch(mimeType.String(), 1, const_cast<char**>(args));
		}
	} else if (Attribute() == "META:email") {
		if (argument.IFindFirst("mailto:") != 0 && argument != "")
			argument.Prepend("mailto:");

		// TODO: Could check for possible e-mail patterns.
		if (argument != "") {
			const char *args[] = {argument.String(), 0};
			be_roster->Launch("text/x-email", 1, const_cast<char**>(args));
		}
	}
}


const BString&
AttributeTextControl::_MakeUniformUrl(BString &url) const
{
	if (url.StartsWith("www"))
		url.Prepend("http://");
	else if (url.StartsWith("ftp."))
		url.Prepend("ftp://");

	return url;
}


const BString&
AttributeTextControl::_BuildMimeString(BString &mimeType, const BString &url)
	const
{
	if (url.IFindFirst("http://") == 0 || url.IFindFirst("ftp://") == 0
		|| url.IFindFirst("https://") || url.IFindFirst("gopher://") == 0) {

		mimeType.Append(url, url.FindFirst(':'));
	}

	if (!BMimeType::IsValid(mimeType.String()))
		mimeType = "";

	return mimeType;
}


bool
AttributeTextControl::_ContainsUrl() const
{
	BString argument(Text());

	if (Attribute() == "META:url") {
		BString mimeType = B_URL_MIME;
		if (_BuildMimeString(mimeType, _MakeUniformUrl(argument)) != "")
			return true;
	}
	else if (Attribute() == "META:email") {
		if (argument != "")
			return true;
	}

	return false;
}


BRect
AttributeTextControl::_VisibleLabelBounds() const
{
	// TODO: Could actually check current alignment of the label.
	// The code below works only for right aligned labels.
	BRect bounds(Bounds());
	bounds.right = Divider();
	bounds.left = bounds.right - StringWidth(Label());
	return bounds;
}
