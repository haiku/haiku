/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Davidson, slaad@bong.com.au
 *		Mikael Eiman, mikael@eiman.tv
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <algorithm>

#include <GroupLayout.h>
#include <GroupView.h>

#include "AppGroupView.h"

#include "NotificationWindow.h"
#include "NotificationView.h"


AppGroupView::AppGroupView(NotificationWindow* win, const char* label)
	:
	BBox(B_FANCY_BORDER, (fView = new BGroupView(B_VERTICAL, 10))),
	fLabel(label),
	fParent(win),
	fCollapsed(false)
{
	// If no group was specified we don't have any border or label
	if (label == NULL)
		SetBorder(B_NO_BORDER);
	else
		SetLabel(label);
}


AppGroupView::~AppGroupView()
{
}


/*
void
AppGroupView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
}
*/

/*
void
AppGroupView::Draw(BRect updateRect)
{
	FillRect(Bounds(), B_SOLID_LOW);

	BString label = fLabel;
	if (fCollapsed)
		label << " (" << fInfo.size() << ")";

	font_height fh;
	be_bold_font->GetHeight(&fh);
	float labelOffset = fh.ascent + fh.leading;


	BRect textRect = Bounds();
	//textRect.right = textRect.left + be_bold_font->StringWidth(label.String())
	//	+ (kEdgePadding * 3);
	textRect.bottom = 2 * labelOffset;
	
	BRect borderRect = Bounds().InsetByCopy(kEdgePadding, kEdgePadding);
	borderRect.top = 2 * labelOffset;

	BRect closeCross = fCloseRect;
	closeCross.InsetBy(kSmallPadding, kSmallPadding);

	rgb_color detailCol = ui_color(B_CONTROL_BORDER_COLOR);
	detailCol = tint_color(detailCol, B_LIGHTEN_2_TINT);
	// detailCol = tint_color(detailCol, B_LIGHTEN_1_TINT);

	PushState();
	SetFont(be_bold_font);
	SetPenSize(kPenSize);

	if (fCollapsed) {
		// Draw the expand widget
		PushState();
			SetHighColor(detailCol);
			StrokeRoundRect(fCollapseRect, kSmallPadding, kSmallPadding);
			
			BPoint expandHorStart(fCollapseRect.left + kSmallPadding, fCollapseRect.Height() / 2 + fCollapseRect.top);
			BPoint expandHorEnd(fCollapseRect.right - kSmallPadding, fCollapseRect.Height() / 2 + fCollapseRect.top);			
			StrokeLine(expandHorStart, expandHorEnd);
			
			BPoint expandVerStart(fCollapseRect.Width() / 2 + fCollapseRect.left, fCollapseRect.top + kSmallPadding);
			BPoint expandVerEnd(fCollapseRect.Width() / 2 + fCollapseRect.left, fCollapseRect.bottom - kSmallPadding);				
			StrokeLine(expandVerStart, expandVerEnd);
		PopState();
		
		SetHighColor(tint_color(ui_color(B_PANEL_TEXT_COLOR), B_LIGHTEN_1_TINT));
	} else {
		SetLowColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
		FillRect(textRect, B_SOLID_LOW);
			
		SetHighColor(ui_color(B_PANEL_TEXT_COLOR));

		// Draw the collapse widget
		StrokeRoundRect(fCollapseRect, kSmallPadding, kSmallPadding);

		BPoint expandHorStart(fCollapseRect.left + kSmallPadding, fCollapseRect.Height() / 2 + fCollapseRect.top);
		BPoint expandHorEnd(fCollapseRect.right - kSmallPadding, fCollapseRect.Height() / 2 + fCollapseRect.top);

		StrokeLine(expandHorStart, expandHorEnd);
	}
	// Draw the dismiss widget
	StrokeRoundRect(fCloseRect, kSmallPadding, kSmallPadding);

	StrokeLine(closeCross.LeftTop(), closeCross.RightBottom());
	StrokeLine(closeCross.RightTop(), closeCross.LeftBottom());

	// Draw the label
	DrawString(label.String(), BPoint(fCollapseRect.right + 2 * kEdgePadding, labelOffset + kEdgePadding));
	PopState();

	Sync();	
}
*/

void
AppGroupView::MouseDown(BPoint point)
{
	bool changed = false;
	if (fCloseRect.Contains(point)) {
		changed = true;

		int32 children = fInfo.size();
		for (int32 i = 0; i < children; i++) {
			fView->GetLayout()->RemoveView(fInfo[i]);
			delete fInfo[i];
		}
		fInfo.clear();
	}

	if (fCollapseRect.Contains(point)) {
		fCollapsed = !fCollapsed;
		changed = true;
	}

	if (changed) {
		_ResizeViews();
	}
}


void
AppGroupView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kRemoveView:
		{
			NotificationView* view = NULL;
			if (msg->FindPointer("view", (void**)&view) != B_OK)
				return;

			infoview_t::iterator vIt = find(fInfo.begin(), fInfo.end(), view);

			if (vIt != fInfo.end()) {
				fInfo.erase(vIt);
				fView->GetLayout()->RemoveView(view);
				delete view;
			}

			if (Window() != NULL)
				Window()->PostMessage(msg);

			_ResizeViews();
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}


void
AppGroupView::AddInfo(NotificationView* view)
{
	BString id = view->MessageID();
	bool found = false;

	if (id.Length() > 0) {
		int32 children = fInfo.size();

		for (int32 i = 0; i < children; i++) {
			if (id == fInfo[i]->MessageID()) {
				fView->GetLayout()->RemoveView(fInfo[i]);
				delete fInfo[i];
				
				fInfo[i] = view;
				found = true;
				
				break;
			}
		}
	}

	if (!found)
		fInfo.push_back(view);
	fView->GetLayout()->AddView(view);

	if (IsHidden())
		Show();
	if (view->IsHidden())
		view->Show();

	_ResizeViews();
}


void
AppGroupView::_ResizeViews()
{
	font_height fh;
	be_bold_font->GetHeight(&fh);

	float offset = 2 * kEdgePadding + fh.ascent + fh.leading + fh.descent;
	int32 children = fInfo.size();

	if (!fCollapsed) {
		offset += kEdgePadding + kPenSize;

		for (int32 i = 0; i < children; i++) {
			fInfo[i]->ResizeToPreferred();
			fInfo[i]->MoveTo(0, offset);
			
			offset += fInfo[i]->Bounds().Height();
			if (fInfo[i]->IsHidden())
				fInfo[i]->Show();
		}
	} else {
		for (int32 i = 0; i < children; i++)
			if (!fInfo[i]->IsHidden())
				fInfo[i]->Hide();
	}

	ResizeTo(fParent->Width(), offset);
	float labelOffset = fh.ascent + fh.leading;

	BRect borderRect = Bounds().InsetByCopy(kEdgePadding, kEdgePadding);
	borderRect.top = 2*labelOffset;

	fCollapseRect = borderRect;
	fCollapseRect.right = fCollapseRect.left + kExpandSize;
	fCollapseRect.bottom = fCollapseRect.top + kExpandSize;
	fCollapseRect.OffsetTo(kEdgePadding * 2, kEdgePadding * 1.5);

	fCloseRect = borderRect;
	fCloseRect.right -= kEdgePadding * 2;
	fCloseRect.top += kEdgePadding * 1.5;
	fCloseRect.left = fCloseRect.right - kCloseSize;
	fCloseRect.bottom = fCloseRect.top + kCloseSize;
}


bool
AppGroupView::HasChildren()
{
	return !fInfo.empty();
}
