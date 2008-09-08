/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */
#include "StatusView.h"

#include <math.h>
#include <stdio.h>

#include <Box.h>
#include <Node.h>
#include <String.h>
#include <StringView.h>

#include "Common.h"
#include "Scanner.h"


StatusView::StatusView(BRect rect)
	: BView(rect, NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW),
	  fCurrentFileInfo(NULL)
{
	SetViewColor(kWindowColor);

	struct font_height fh;
	be_plain_font->GetHeight(&fh);
	float plainHeight = ceilf(fh.ascent) + ceilf(fh.descent)
		+ ceilf(fh.leading);
	float fixedHeight = ceilf(fh.ascent) + ceilf(fh.descent)
		+ ceilf(fh.leading);
	ResizeTo(Bounds().Width(),
		2.0 * kSmallVMargin + max_c(plainHeight, fixedHeight));

	BRect r = Bounds();
	r.top += kSmallVMargin;

	// Size display
	r.right -= kSmallHMargin;
	r.left = r.right - StringWidth("9999.99 GB");
	fSizeView = new BStringView(r, NULL, kEmptyStr,
		B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM);
	AddChild(fSizeView);

	// Vertical divider
	r.right = r.left - kSmallHMargin;
	r.left = r.right - 1.0;
	AddChild(new BBox(r.InsetByCopy(0, -5), NULL,
		B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM));

	// Count display
	char testLabel[256];
	sprintf(testLabel, kManyFiles, 999999);
	r.right = r.left - kSmallHMargin;
	r.left = r.right - (StringWidth(testLabel) + kSmallHMargin);
	fCountView = new BStringView(r, NULL, kEmptyStr,
		B_FOLLOW_RIGHT|B_FOLLOW_TOP_BOTTOM);
	AddChild(fCountView);

	// Vertical divider
	r.right = r.left - kSmallHMargin;
	r.left = r.right - 1.0;
	AddChild(new BBox(r.InsetByCopy(0, -5), NULL,
		B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM));

	// Path display
	r.right = r.left - kSmallHMargin;
	r.left = kSmallHMargin;
	fPathView = new BStringView(r, NULL, kEmptyStr,
		B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP_BOTTOM);
	AddChild(fPathView);

	// Horizontal divider
	r = Bounds();
	r.bottom = r.top + 1.0;
	AddChild(new BBox(r.InsetByCopy(-5, 0), NULL,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW));
}


StatusView::~StatusView()
{
}


void
StatusView::Show(const FileInfo* info)
{
	if (info == fCurrentFileInfo)
		return;

	fCurrentFileInfo = info;

	if (info == NULL) {
		fPathView->SetText(kEmptyStr);
		fSizeView->SetText(kEmptyStr);
		fCountView->SetText(kEmptyStr);
		return;
	}

	if (!info->pseudo) {
		BNode node(&info->ref);
		if (node.InitCheck() != B_OK) {
			fPathView->SetText(kStrUnavail);
			fSizeView->SetText(kEmptyStr);
			fCountView->SetText(kEmptyStr);
			return;
		}
	}

	float viewWidth = fPathView->Bounds().Width();
	string path;
	info->GetPath(path);
	BString pathLabel = path.c_str();
	be_plain_font->TruncateString(&pathLabel, B_TRUNCATE_BEGINNING, viewWidth);
	fPathView->SetText(pathLabel.String());

	char label[B_PATH_NAME_LENGTH];
	size_to_string(info->size, label);
	fSizeView->SetText(label);

	if (info->count > 0) {
		char label[256];
		sprintf(label, (info->count == 1) ? kOneFile : kManyFiles, info->count);
		fCountView->SetText(label);
	} else {
		fCountView->SetText(kEmptyStr);
	}
}


