/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// DormantNodeListItem.cpp

#include "DormantNodeListItem.h"
// InfoWindow
#include "InfoWindowManager.h"
// Support
#include "MediaIcon.h"
// TipManager
#include "TipManager.h"

// Application Kit
#include <Application.h>
// Locale Kit
#undef B_CATALOG
#define B_CATALOG (&sCatalog)
#include <Catalog.h>
// Interface Kit
#include <PopUpMenu.h>
#include <MenuItem.h>
// Media Kit
#include <MediaRoster.h>
#include <MediaAddOn.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CortexDormantNodeListItem"

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_ALLOC(x) //PRINT (x)		// ctor/dtor
#define D_HOOK(x) //PRINT (x)		// BListItem impl.
#define D_OPERATION(x) //PRINT (x)	// operations
#define D_COMPARE(x) //PRINT (x)	// compare functions

static BCatalog sCatalog("x-vnd.Cortex.DormantNodeView");

// -------------------------------------------------------- //
// constants
// -------------------------------------------------------- //

const float DormantNodeListItem::M_ICON_H_MARGIN		= 3.0;
const float DormantNodeListItem::M_ICON_V_MARGIN		= 1.0;

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

DormantNodeListItem::DormantNodeListItem(
	const dormant_node_info &nodeInfo)
	: m_info(nodeInfo),
	  m_icon(0) {
	D_ALLOC(("DormantNodeListItem::DormantNodeListItem()\n"));

	m_icon = new MediaIcon(nodeInfo, B_MINI_ICON);
}

DormantNodeListItem::~DormantNodeListItem() {
	D_ALLOC(("DormantNodeListItem::~DormantNodeListItem()\n"));

	delete m_icon;
}
	
// -------------------------------------------------------- //
// BListItem impl.
// -------------------------------------------------------- //

void DormantNodeListItem::DrawItem(
	BView *owner,
	BRect frame,
	bool drawEverything) {
	D_HOOK(("DormantNodeListItem::DrawItem()\n"));

	// Draw icon
	if (m_icon) {
		BRect r = frame;
		r.left += M_ICON_H_MARGIN;
		r.top += (frame.Height() / 2.0) - (B_MINI_ICON / 2.0);
		r.right = r.left + B_MINI_ICON - 1.0;
		r.bottom = r.top + B_MINI_ICON - 1.0;
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(m_icon, r.LeftTop());
	}

	// Draw label
	BRect r = frame;
	r.left += 2 * M_ICON_H_MARGIN + B_MINI_ICON;
	r.top += (frame.Height() / 2.0) - (m_fontHeight.ascent / 2.0);
	r.right = r.left + Width();
	r.bottom = r.top + m_fontHeight.ascent + m_fontHeight.descent;		
	if (IsSelected() || drawEverything) {
		if (IsSelected()) {
			owner->SetHighColor(16, 64, 96, 255);
		}
		else {
			owner->SetHighColor(owner->ViewColor());
		}
		owner->FillRect(r);
	}
	if (IsSelected()) {
		owner->SetHighColor(255, 255, 255, 255);
	}
	else {
		owner->SetHighColor(0, 0, 0, 0);
	}
	BPoint labelOffset(r.left + 1.0, r.bottom - m_fontHeight.descent);
	owner->DrawString(m_info.name, labelOffset);

	// cache the frame rect
	m_frame = frame;
}

void DormantNodeListItem::Update(
	BView *owner,
	const BFont *font) {
	D_HOOK(("DormantNodeListItem::Update()\n"));
	BListItem::Update(owner, font);

	SetWidth(font->StringWidth(m_info.name));
	owner->GetFontHeight(&m_fontHeight);
	float newHeight = m_fontHeight.ascent + m_fontHeight.descent + m_fontHeight.leading;
	if (newHeight < B_MINI_ICON) {
		newHeight = B_MINI_ICON;
	}
	newHeight += 2 * M_ICON_V_MARGIN;
	if (Height() < newHeight) {
		SetHeight(newHeight);
	}
}

// -------------------------------------------------------- //
// BListItem impl.
// -------------------------------------------------------- //

void DormantNodeListItem::MouseOver(
	BView *owner,
	BPoint point,
	uint32 transit) {
	D_OPERATION(("DormantNodeListItem::MouseOver()\n"));

	switch (transit) {
		case B_ENTERED_VIEW: {
			TipManager *tips = TipManager::Instance();
			dormant_flavor_info flavorInfo;
			BMediaRoster *roster = BMediaRoster::CurrentRoster();
			if (roster && roster->GetDormantFlavorInfoFor(info(), &flavorInfo) == B_OK) {
				BString tip = flavorInfo.info;
				if (tip == "") {
					tip = m_info.name;
				}
				tips->showTip(tip.String(), owner->ConvertToScreen(m_frame));
			}
			break;
		}
		case B_EXITED_VIEW: {
			TipManager *tips = TipManager::Instance();
			tips->hideTip(owner->ConvertToScreen(m_frame));
			break;
		}
	}
}

BRect DormantNodeListItem::getRealFrame(
	const BFont *font) const {
	D_OPERATION(("DormantNodeListItem::getRealFrame()\n"));

	BRect r(0.0, 0.0, -1.0, -1.0);
	if (m_frame.IsValid()) {
		r = m_frame;
		r.right = r.left + B_MINI_ICON + 2 * M_ICON_H_MARGIN;
		r.right += font->StringWidth(m_info.name);
	}
	else {
		r.right = font->StringWidth(m_info.name);
		r.right += B_MINI_ICON + 2 * M_ICON_H_MARGIN + 5.0;
		font_height fh;
		font->GetHeight(&fh);
		r.bottom = fh.ascent + fh.descent + fh.leading;
		if (r.bottom < B_MINI_ICON) {
			r.bottom = B_MINI_ICON;
		}
		r.bottom += 2 * M_ICON_V_MARGIN;
		if (Height() > r.bottom) {
			r.bottom = Height();
		}
	}
	return r;
}

BBitmap *DormantNodeListItem::getDragBitmap() {
	D_OPERATION(("DormantNodeListItem::getDragBitmap()\n"));

	// Prepare Bitmap for Drag & Drop
	BBitmap *dragBitmap = new BBitmap(m_frame.OffsetToCopy(BPoint(0.0, 0.0)), B_RGBA32, true); 
	dragBitmap->Lock(); {
		BView *dragView = new BView(dragBitmap->Bounds(), "", B_FOLLOW_NONE, 0); 
		dragBitmap->AddChild(dragView); 
		dragView->SetOrigin(0.0, 0.0);
		dragView->SetHighColor(0, 0, 0, 0);
		dragView->FillRect(dragView->Bounds()); 
		dragView->SetDrawingMode(B_OP_ALPHA); 
		dragView->SetHighColor(0, 0, 0, 128);       
	    dragView->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE); 
		// Drawing code back again
		if (m_icon) {
			BRect r = dragView->Bounds();
			r.left += M_ICON_H_MARGIN;
			r.top += (m_frame.Height() / 2.0) - (B_MINI_ICON / 2.0);
			r.right = r.left + B_MINI_ICON - 1.0;
			r.bottom = r.top + B_MINI_ICON - 1.0;
			dragView->DrawBitmap(m_icon, r.LeftTop());
		}
		BRect r = dragView->Bounds();
		r.left += 2 * M_ICON_H_MARGIN + B_MINI_ICON;
		r.top += (m_frame.Height() / 2.0) - (m_fontHeight.ascent / 2.0);
		r.right = r.left + Width();
		r.bottom = r.top + m_fontHeight.ascent + m_fontHeight.descent;		
		BPoint labelOffset(r.left + 1.0, r.bottom - m_fontHeight.descent);
		dragView->DrawString(m_info.name, labelOffset);
		dragView->Sync();
	}
	dragBitmap->Unlock();
	return dragBitmap;
}

void DormantNodeListItem::showContextMenu(
	BPoint point,
	BView *owner) {
	D_OPERATION(("DormantNodeListItem::showContextMenu()\n"));

	BPopUpMenu *menu = new BPopUpMenu("DormantNodeListItem PopUp", false, false, B_ITEMS_IN_COLUMN);
	menu->SetFont(be_plain_font);
	
	// Add the "Get Info" item
	BMessage *message = new BMessage(InfoWindowManager::M_INFO_WINDOW_REQUESTED);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Get info"), message));

	menu->SetTargetForItems(owner);
	owner->ConvertToScreen(&point);
	point -= BPoint(1.0, 1.0);
	menu->Go(point, true, true, true);
}

// -------------------------------------------------------- //
// *** compare functions (friend)
// -------------------------------------------------------- //

int __CORTEX_NAMESPACE__ compareName(
	const void *lValue,
	const void *rValue) {
	D_COMPARE(("compareSelectionTime()\n"));

	int returnValue = 0;
	const DormantNodeListItem *lItem = *(reinterpret_cast<const DormantNodeListItem* const*>
										(reinterpret_cast<const void* const*>(lValue)));
	const DormantNodeListItem *rItem = *(reinterpret_cast<const DormantNodeListItem* const*>
										(reinterpret_cast<const void* const*>(rValue)));
	BString lString = lItem->info().name;
	lString.ToUpper();
	BString rString = rItem->info().name;
	rString.ToUpper();
	if (lString < rString) {
		returnValue = -1;
	}
	else if (lString > rString) {
		returnValue = 1;
	}
	return returnValue;
}

int __CORTEX_NAMESPACE__ compareAddOnID(
	const void *lValue,
	const void *rValue) {
	D_COMPARE(("compareAddOnID()\n"));

	int returnValue = 0;
	const DormantNodeListItem *lItem = *(reinterpret_cast<const DormantNodeListItem* const*>
										(reinterpret_cast<const void* const*>(lValue)));
	const DormantNodeListItem *rItem = *(reinterpret_cast<const DormantNodeListItem* const*>
										(reinterpret_cast<const void* const*>(rValue)));
	if (lItem->info().addon < rItem->info().addon) {
		returnValue = -1;
	}
	else if (lItem->info().addon > rItem->info().addon) {
		returnValue = 1;
	}
	return returnValue;
}

// END -- DormantNodeListItem.cpp --
