/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "IconView.h"

#include <Application.h>
#include <Bitmap.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <PopUpMenu.h>
#include <Resources.h>

#include <string.h>


IconView::IconView(BRect rect, const char* name, uint32 resizeMode)
	: BView(rect, name, resizeMode, B_WILL_DRAW),
	fIcon(NULL),
	fHeapIcon(NULL)
{
}


IconView::~IconView()
{
}


void
IconView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
IconView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_ALPHA);

	if (fHeapIcon != NULL)
		DrawBitmap(fHeapIcon, BPoint(0.0f, 0.0f));
	else if (fIcon != NULL)
		DrawBitmap(fIcon, BPoint(0.0f, 0.0f));
	else {
		// draw frame so that the user knows here is something he
		// might be able to click on
		SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
		StrokeRect(Bounds());
	}
}


void
IconView::MouseDown(BPoint where)
{
	int32 buttons = B_PRIMARY_MOUSE_BUTTON;
	if (Looper() != NULL && Looper()->CurrentMessage() != NULL)
		Looper()->CurrentMessage()->FindInt32("buttons", &buttons);

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		// show context menu

		ConvertToScreen(&where);

		BPopUpMenu* menu = new BPopUpMenu("context");
		menu->SetFont(be_plain_font);
		BMenuItem* item;
		menu->AddItem(item = new BMenuItem(fIcon != NULL
			? "Edit Icon" B_UTF8_ELLIPSIS : "Add Icon" B_UTF8_ELLIPSIS, NULL));
		item->SetEnabled(false);
		menu->AddItem(item = new BMenuItem("Remove Icon", NULL));
		item->SetEnabled(false);

		menu->Go(where);
	}
}


void
IconView::SetTo(entry_ref* ref)
{
	delete fIcon;
	fIcon = NULL;

	if (ref != NULL) {
		BNode node(ref);
		BNodeInfo info(&node);
		if (node.InitCheck() != B_OK
			|| info.InitCheck() != B_OK)
			return;
	
		BBitmap* icon = new BBitmap(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_CMAP8);
		if (info.GetIcon(icon, B_LARGE_ICON) != B_OK) {
			delete icon;
			return;
		}

		fIcon = icon;
	}

	Invalidate();
}


void
IconView::ShowIconHeap(bool show)
{
	if (show == (fHeapIcon != NULL))
		return;

	if (show) {
		BResources* resources = be_app->AppResources();
		const void* data = NULL;
		if (resources != NULL)
			data = resources->LoadResource('ICON', "icon heap", NULL);
		if (data != NULL) {
			fHeapIcon = new BBitmap(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1),
				B_CMAP8);
			memcpy(fHeapIcon->Bits(), data, fHeapIcon->BitsLength());
		}
	} else {
		delete fHeapIcon;
		fHeapIcon = NULL;
	}
}

