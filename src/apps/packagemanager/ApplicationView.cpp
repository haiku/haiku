/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (C) 2010 Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 *
 * Distributed under the terms of the MIT licence.
 */

#include "ApplicationView.h"

#include <stdio.h>

#include <Alert.h>
#include <Bitmap.h>
#include <Button.h>
#include <Clipboard.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <GroupLayoutBuilder.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>
#include <StringView.h>


class IconView : public BView {
public:
	IconView(const BEntry& entry)
		:
		BView("Download icon", B_WILL_DRAW),
		fIconBitmap(BRect(0, 0, 31, 31), 0, B_RGBA32),
		fDimmedIcon(false)
	{
		SetDrawingMode(B_OP_OVER);
		SetTo(entry);
	}

	IconView()
		:
		BView("Download icon", B_WILL_DRAW),
		fIconBitmap(BRect(0, 0, 31, 31), 0, B_RGBA32),
		fDimmedIcon(false)
	{
		SetDrawingMode(B_OP_OVER);
		memset(fIconBitmap.Bits(), 0, fIconBitmap.BitsLength());
	}

	IconView(const BMessage* archive)
		:
		BView("Download icon", B_WILL_DRAW),
		fIconBitmap((BMessage*)archive),
		fDimmedIcon(true)
	{
		SetDrawingMode(B_OP_OVER);
	}

	void SetTo(const BEntry& entry)
	{
		BNode node(&entry);
		BNodeInfo info(&node);
		info.GetTrackerIcon(&fIconBitmap, B_LARGE_ICON);
		Invalidate();
	}

	void SetIconDimmed(bool iconDimmed)
	{
		if (fDimmedIcon != iconDimmed) {
			fDimmedIcon = iconDimmed;
			Invalidate();
		}
	}

	bool IsIconDimmed() const
	{
		return fDimmedIcon;
	}

	status_t SaveSettings(BMessage* archive)
	{
		return fIconBitmap.Archive(archive);
	}

	virtual void AttachedToWindow()
	{
		SetViewColor(Parent()->ViewColor());
	}

	virtual void Draw(BRect updateRect)
	{
		if (fDimmedIcon) {
			SetDrawingMode(B_OP_ALPHA);
			SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
			SetHighColor(0, 0, 0, 100);
		}
		DrawBitmapAsync(&fIconBitmap);
	}

	virtual BSize MinSize()
	{
		return BSize(fIconBitmap.Bounds().Width(), fIconBitmap.Bounds().Height());
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

	virtual BSize MaxSize()
	{
		return MinSize();
	}

private:
	BBitmap	fIconBitmap;
	bool	fDimmedIcon;
};


class SmallButton : public BButton {
public:
	SmallButton(const char* label, BMessage* message = NULL)
		:
		BButton(label, message)
	{
		BFont font;
		GetFont(&font);
		float size = ceilf(font.Size() * 0.8);
		font.SetSize(max_c(8, size));
		SetFont(&font, B_FONT_SIZE);
	}
};


// #pragma mark - ApplicationView


ApplicationView::ApplicationView(const char* name, const char* icon,
		const char* description)
	:
	BGroupView(B_HORIZONTAL, 8)
{
	Init(NULL);

	fNameView->SetText(name);
	fInfoView->SetText(description);
}


ApplicationView::ApplicationView(const BMessage* archive)
	:
	BGroupView(B_HORIZONTAL, 8)
{
	Init(archive);
	BString temp;
	archive->FindString("appname",&temp);
	fNameView->SetText(temp);
	archive->FindString("appdesc",&temp);
	float ver = 0.0;
	archive->FindFloat("appver", ver);
	int size = 0;
	archive->FindInt32("appsize", size);
	temp << " Version: " << ver << " Size: " << size;
	fInfoView->SetText(temp);
}


bool
ApplicationView::Init(const BMessage* archive)
{
	SetViewColor(245, 245, 245);
	SetFlags(Flags() | B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW);

	if (archive) {
			fIconView = new IconView(archive);
	} else
		fIconView = new IconView();

	if (archive) {
		fTopButton = new SmallButton("Info", new BMessage('NADA'));
		fBottomButton = new SmallButton("Install", new BMessage('NADA'));
	}

	fInfoView = new BStringView("info view", "");
	fNameView = new BStringView("name view", "");

	BGroupLayout* layout = GroupLayout();
	layout->SetInsets(8, 5, 5, 6);
	layout->AddView(fIconView);
	BView* verticalGroup = BGroupLayoutBuilder(B_VERTICAL, 3)
		.Add(fNameView)
		.Add(fInfoView)
		.TopView()
	;
	verticalGroup->SetViewColor(ViewColor());
	layout->AddView(verticalGroup);
	if (fTopButton && fBottomButton) {
		verticalGroup = BGroupLayoutBuilder(B_VERTICAL, 3)
			.Add(fTopButton)
			.Add(fBottomButton)
			.TopView()
		;
	}
	verticalGroup->SetViewColor(ViewColor());
	layout->AddView(verticalGroup);

	BFont font;
	fInfoView->GetFont(&font);
	float fontSize = font.Size() * 0.8f;
	font.SetSize(max_c(8.0f, fontSize));
	fInfoView->SetFont(&font, B_FONT_SIZE);
	fInfoView->SetHighColor(tint_color(fInfoView->LowColor(),
		B_DARKEN_4_TINT));
	fInfoView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fNameView->GetFont(&font);
	font.SetSize(font.Size() * 2.0f);
	fNameView->SetFont(&font, B_FONT_SIZE);
	fNameView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	return true;
}


void
ApplicationView::AttachedToWindow()
{
	if (fTopButton)
		fTopButton->SetTarget(this);
	if (fBottomButton)
		fBottomButton->SetTarget(this);
}


void
ApplicationView::AllAttached()
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(245, 245, 245);
	SetHighColor(tint_color(LowColor(), B_DARKEN_1_TINT));
}


void
ApplicationView::Draw(BRect updateRect)
{
	BRect bounds(Bounds());
	bounds.bottom--;
	FillRect(bounds, B_SOLID_LOW);
	bounds.bottom++;
	StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
}


void
ApplicationView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
		{
			int32 opCode;
			if (message->FindInt32("opcode", &opCode) != B_OK)
				break;
			switch (opCode) {
				case B_ENTRY_MOVED:
				{
					// Follow the entry to the new location
					dev_t device;
					ino_t directory;
					const char* name;
					if (message->FindInt32("device",
							reinterpret_cast<int32*>(&device)) != B_OK
						|| message->FindInt64("to directory",
							reinterpret_cast<int64*>(&directory)) != B_OK
						|| message->FindString("name", &name) != B_OK
						|| strlen(name) == 0) {
						break;
					}

					Window()->PostMessage(SAVE_SETTINGS);
					break;
				}
				case B_ATTR_CHANGED:
				{
					fIconView->SetIconDimmed(false);
					break;
				}
			}
			break;
		}

		default:
			BGroupView::MessageReceived(message);
	}
}


void
ApplicationView::ShowContextMenu(BPoint screenWhere)
{
	screenWhere += BPoint(2, 2);

	BPopUpMenu* contextMenu = new BPopUpMenu("download context");
	BMenuItem* copyURL = new BMenuItem("Copy URL to clipboard",
		new BMessage('NADA'));
	contextMenu->AddItem(copyURL);
	BMenuItem* openFolder = new BMenuItem("Open containing folder",
		new BMessage('NADA'));
	contextMenu->AddItem(openFolder);

	contextMenu->SetTargetForItems(this);
	contextMenu->Go(screenWhere, true, true, true);
}

