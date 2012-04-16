/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */


#include "InfoWindow.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <utility>
#include <vector>

#include <Catalog.h>
#include <StringView.h>
#include <Bitmap.h>
#include <NodeInfo.h>

#include "DiskUsage.h"
#include "Snapshot.h"

using std::string;
using std::vector;
using std::pair;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Info Window"

LeftView::LeftView(BRect frame, BBitmap* icon)
	:
	BView(frame, NULL, B_FOLLOW_NONE, B_WILL_DRAW),
	fIcon(icon)
{
	SetViewColor(tint_color(kWindowColor, B_LIGHTEN_1_TINT));
}


LeftView::~LeftView()
{
	delete fIcon;
}


void
LeftView::Draw(BRect updateRect)
{
	float right = Bounds().Width() - kSmallHMargin;
	BRect iconRect(right - 31.0, kSmallVMargin, right, kSmallVMargin + 31.0);
	if (updateRect.Intersects(iconRect)) {
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(fIcon, iconRect);
	}
}


InfoWin::InfoWin(BPoint p, FileInfo *f, BWindow* parent)
	: BWindow(BRect(p, p), kEmptyStr, B_FLOATING_WINDOW_LOOK,
		B_FLOATING_SUBSET_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE)
{
	AddToSubset(parent);

	typedef pair<string, string> Item;
	typedef vector<Item> InfoList;

	BString stringTitle("%refName% info");
	stringTitle.ReplaceFirst("%refName%", f->ref.name);
	SetTitle(stringTitle.String());

	InfoList info;
	Item item;

	// Size
	char name[B_PATH_NAME_LENGTH] = { 0 };
	size_to_string(f->size, name, sizeof(name));
	if (f->count > 0) {
		// This is a directory.
		char str[64];
		snprintf(str, sizeof(str), B_TRANSLATE(" in %d files"),
			f->count);
		strlcat(name, str, sizeof(name));
	}
	info.push_back(Item(B_TRANSLATE_MARK("Size"), name));

	// Created & modified dates
	BEntry entry(&f->ref);
	time_t t;
	entry.GetCreationTime(&t);
	strftime(name, 64, B_TRANSLATE("%a, %d %b %Y, %r"), localtime(&t));
	info.push_back(Item(B_TRANSLATE("Created"), name));
	entry.GetModificationTime(&t);
	strftime(name, 64, B_TRANSLATE("%a, %d %b %Y, %r"), localtime(&t));
	info.push_back(Item(B_TRANSLATE("Modified"), name));

	// Kind
	BMimeType* type = f->Type();
	type->GetShortDescription(name);
	info.push_back(Item(B_TRANSLATE("Kind"), name));
	delete type;

	// Path
	string path;
	f->GetPath(path);
	info.push_back(Item(B_TRANSLATE("Path"), path));

	// Icon
	BBitmap *icon = new BBitmap(BRect(0.0, 0.0, 31.0, 31.0), B_RGBA32);
	entry_ref ref;
	entry.GetRef(&ref);
	BNodeInfo::GetTrackerIcon(&ref, icon, B_LARGE_ICON);

	// Compute the window size and add the views.
	BFont smallFont(be_plain_font);
	smallFont.SetSize(floorf(smallFont.Size() * 0.95));

	struct font_height fh;
	smallFont.GetHeight(&fh);
	float fontHeight = fh.ascent + fh.descent + fh.leading;

	float leftWidth = 32.0;
	float rightWidth = 0.0;
	InfoList::iterator i = info.begin();
	while (i != info.end()) {
		float w = smallFont.StringWidth((*i).first.c_str()) + 
			2.0 * kSmallHMargin;
		leftWidth = max_c(leftWidth, w);
		w = smallFont.StringWidth((*i).second.c_str()) + 2.0 * kSmallHMargin;
		rightWidth = max_c(rightWidth, w);
		i++;
	}

	float winHeight = 32.0 + 4.0 * kSmallVMargin + 5.0 * (fontHeight 
		+ kSmallVMargin);
	float winWidth = leftWidth + rightWidth;
	ResizeTo(winWidth, winHeight);

	LeftView *leftView = new LeftView(BRect(0.0, 0.0, leftWidth, winHeight),
		icon);
	BView *rightView = new BView(
		BRect(leftWidth + 1.0, 0.0, winWidth, winHeight), NULL,
		B_FOLLOW_NONE, B_WILL_DRAW);

	AddChild(leftView);
	AddChild(rightView);

	BStringView *sv = new BStringView(
		BRect(kSmallHMargin, kSmallVMargin, rightView->Bounds().Width(),
		kSmallVMargin + 30.0), NULL, f->ref.name, B_FOLLOW_ALL);

	BFont largeFont(be_plain_font);
	largeFont.SetSize(ceilf(largeFont.Size() * 1.1));
	sv->SetFont(&largeFont);
	rightView->AddChild(sv);

	float y = 32.0 + 4.0 * kSmallVMargin;
	i = info.begin();
	while (i != info.end()) {
		sv = new BStringView(
			BRect(kSmallHMargin, y, leftView->Bounds().Width(),
			y + fontHeight), NULL, (*i).first.c_str());
		sv->SetFont(&smallFont);
		sv->SetAlignment(B_ALIGN_RIGHT);
		sv->SetHighColor(kBasePieColor[1]); // arbitrary
		leftView->AddChild(sv);

		sv = new BStringView(
			BRect(kSmallHMargin, y, rightView->Bounds().Width(),
			y + fontHeight), NULL, (*i).second.c_str());
		sv->SetFont(&smallFont);
		rightView->AddChild(sv);

		y += fontHeight + kSmallVMargin;
		i++;
	}

	Show();
}


InfoWin::~InfoWin()
{
}
