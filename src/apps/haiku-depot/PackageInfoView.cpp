/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageInfoView.h"

#include <algorithm>
#include <stdio.h>

#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Message.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageInfoView"


class TitleView : public BView {
public:
	TitleView()
		:
		BView("title view", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
		fIcon(NULL),
		fTitle()
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		
		// Title font
		BFont font;
		GetFont(&font);
		font.SetSize(font.Size() * 2.0f);
		SetFont(&font);
	}
	
	virtual ~TitleView()
	{
		Clear();
	}

	virtual void Draw(BRect updateRect)
	{
		BPoint offset(B_ORIGIN);
		BRect bounds(Bounds());
		
		FillRect(updateRect, B_SOLID_LOW);
		
		if (fIcon != NULL) {
			// TODO: ...
		}
		
		if (fTitle.Length() > 0) {
			// TODO: Truncate title
			font_height fontHeight;
			GetFontHeight(&fontHeight);

			offset.y = ceilf((bounds.Height() + fontHeight.ascent) / 2.0f);
			DrawString(fTitle.String(), offset);
		}
	}

	virtual BSize MinSize()
	{
		BSize size(0.0f, 0.0f);
		
		if (fTitle.Length() > 0) {
			font_height fontHeight;
			GetFontHeight(&fontHeight);

			size.width = StringWidth(fTitle.String());
			size.height = ceilf(fontHeight.ascent + fontHeight.descent);
		}
		
		return size;
	}
	
	void SetPackage(const PackageInfo& package)
	{
		// TODO: Fetch icon
		fTitle = package.Title();
		InvalidateLayout();
		Invalidate();
	}

	void Clear()
	{
		delete fIcon;
		fIcon = NULL;
		fTitle = "";

		if (Parent() != NULL) {
			InvalidateLayout();
			Invalidate();
		}
	}

private:
	BBitmap*	fIcon;
	BString		fTitle;
};


class PagesView : public BView {
public:
	PagesView()
		:
		BView("pages view", B_WILL_DRAW)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}
	
	virtual ~PagesView()
	{
		Clear();
	}

	virtual void Draw(BRect updateRect)
	{
	}

	void SetPackage(const PackageInfo& package)
	{
	}

	void Clear()
	{
	}

private:
};


PackageInfoView::PackageInfoView()
	:
	BGroupView("package info view", B_VERTICAL)
{
	fTitleView = new TitleView();
	fPagesView = new PagesView();

	BLayoutBuilder::Group<>(this)
		.Add(fTitleView)
		.Add(fPagesView)
	;
}


PackageInfoView::~PackageInfoView()
{
}


void
PackageInfoView::AttachedToWindow()
{
}


void
PackageInfoView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
PackageInfoView::SetPackage(const PackageInfo& package)
{
	fTitleView->SetPackage(package);
	fPagesView->SetPackage(package);
}


void
PackageInfoView::Clear()
{
	fTitleView->Clear();
	fPagesView->Clear();
}

