/*
 * Copyright (c) 2010 Philippe St-Pierre <stpere@gmail.com>. All rights reserved.
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

#include <Catalog.h>
#include <Box.h>
#include <Button.h>
#include <Node.h>
#include <String.h>
#include <StringView.h>

#include <LayoutBuilder.h>

#include "DiskUsage.h"
#include "Scanner.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Status View"

StatusView::StatusView()
	:
	BView(NULL, B_WILL_DRAW),
	fCurrentFileInfo(NULL)
{
	SetViewColor(kPieBGColor);
	SetLowColor(kPieBGColor);

	fSizeView = new BStringView(NULL, kEmptyStr);
	fSizeView->SetExplicitMinSize(BSize(StringWidth(B_TRANSLATE("9999.99 GB")),
		B_SIZE_UNSET));
	fSizeView->SetExplicitMaxSize(BSize(StringWidth(B_TRANSLATE("9999.99 GB")),
		B_SIZE_UNSET));

	char testLabel[256];
	snprintf(testLabel, sizeof(testLabel), B_TRANSLATE("%d files"),
		999999);

	fCountView = new BStringView(NULL, kEmptyStr);
	float width, height;
	fCountView->GetPreferredSize(&width, &height);
	fCountView->SetExplicitMinSize(BSize(StringWidth(testLabel),
		B_SIZE_UNSET));
	fCountView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, height));

	fPathView = new BStringView(NULL, kEmptyStr);
	fPathView->GetPreferredSize(&width, &height);
	fPathView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, height));

	fRefreshBtn = new BButton(NULL, B_TRANSLATE("Scan"),
		new BMessage(kBtnRescan));

	fRefreshBtn->SetExplicitMaxSize(BSize(B_SIZE_UNSET, B_SIZE_UNLIMITED));

	BBox* divider1 = new BBox(BRect(), B_EMPTY_STRING, B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_FRAME_EVENTS, B_FANCY_BORDER);

	BBox* divider2 = new BBox(BRect(), B_EMPTY_STRING, B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_FRAME_EVENTS, B_FANCY_BORDER);

	divider1->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1));
	divider2->SetExplicitMaxSize(BSize(1, B_SIZE_UNLIMITED));

	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BLayoutBuilder::Group<>(B_HORIZONTAL, 0)
		.AddGroup(B_VERTICAL, 0)
			.Add(fPathView)
			.Add(divider1)
			.AddGroup(B_HORIZONTAL, 0)
				.Add(fCountView)
				.Add(divider2)
				.Add(fSizeView)
				.End()			
			.End()
		.AddStrut(kSmallHMargin)
		.Add(fRefreshBtn)
		.SetInsets(kSmallVMargin, kSmallVMargin, kSmallVMargin, kSmallVMargin)
	);
}


StatusView::~StatusView()
{
}


void
StatusView::SetRescanEnabled(bool enabled)
{
	fRefreshBtn->SetEnabled(enabled);
}


void
StatusView::SetBtnLabel(const char* label)
{
	fRefreshBtn->SetLabel(label);
}


void
StatusView::ShowInfo(const FileInfo* info)
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
			fPathView->SetText(B_TRANSLATE("file unavailable"));
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
	size_to_string(info->size, label, sizeof(label));
	fSizeView->SetText(label);

	if (info->count > 0) {
		char label[256];
		snprintf(label, sizeof(label), (info->count == 1) ?
			B_TRANSLATE("%d file") : B_TRANSLATE("%d files"), info->count);
		fCountView->SetText(label);
	} else {
		fCountView->SetText(kEmptyStr);
	}
}
