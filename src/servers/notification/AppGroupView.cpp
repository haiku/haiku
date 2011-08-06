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

#include "AppGroupView.h"

#include "NotificationWindow.h"
#include "NotificationView.h"


AppGroupView::AppGroupView(NotificationWindow* win, const char* label)
	:
	BView(BRect(0, 0, win->ViewWidth(), 1), label, B_FOLLOW_LEFT_RIGHT,
		B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_FRAME_EVENTS),
	fLabel(label),
	fParent(win),
	fCollapsed(false)
{
	Show();
}


AppGroupView::~AppGroupView()
{
}


void
AppGroupView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
}


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
	//textRect.left = kEdgePadding * 2;
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

	if (fCollapsed) {
		PushState();
			SetFont(be_bold_font);
			SetPenSize(kPenSize);
			float linePos = textRect.top + textRect.Height() / 2;

			// Draw the line to the expand widget			
			PushState();
				SetHighColor(detailCol);				
				StrokeLine(BPoint(kEdgePadding, linePos), BPoint(fCollapseRect.left, linePos));
			PopState();
			
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
			
			// Draw the app title
			DrawString(label.String(), BPoint(fCollapseRect.right + kEdgePadding, labelOffset + kEdgePadding));
			
			// Draw the line from the label to the close widget
			PushState();
				SetHighColor(detailCol);
				
				BPoint lineSeg2Start(textRect.right + kSmallPadding / 2, linePos);
				BPoint lineSeg2End(fCloseRect.left, linePos);
				StrokeLine(lineSeg2Start, lineSeg2End);
			PopState();

			// Draw the dismiss widget
			PushState();
				SetHighColor(detailCol);

				StrokeRoundRect(fCloseRect, kSmallPadding, kSmallPadding);

				StrokeLine(closeCross.LeftTop(), closeCross.RightBottom());
				StrokeLine(closeCross.RightTop(), closeCross.LeftBottom());
			PopState();

			// Draw the line from the dismiss widget
			PushState();
				SetHighColor(detailCol);

				BPoint lineSeg3Start(fCloseRect.right, linePos);
				BPoint lineSeg3End(borderRect.right, linePos);
				StrokeLine(lineSeg3Start, lineSeg3End);
			PopState();

		PopState();
	} else {
		PushState();
			SetFont(be_bold_font);
			SetPenSize(kPenSize);

			SetLowColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
			FillRect(textRect, B_SOLID_LOW);
			
			SetHighColor(ui_color(B_PANEL_TEXT_COLOR));

			// Draw the collapse widget
			StrokeRoundRect(fCollapseRect, kSmallPadding, kSmallPadding);

			BPoint expandHorStart(fCollapseRect.left + kSmallPadding, fCollapseRect.Height() / 2 + fCollapseRect.top);
			BPoint expandHorEnd(fCollapseRect.right - kSmallPadding, fCollapseRect.Height() / 2 + fCollapseRect.top);

			StrokeLine(expandHorStart, expandHorEnd);

			// Draw the dismiss widget
			StrokeRoundRect(fCloseRect, kSmallPadding, kSmallPadding);

			StrokeLine(closeCross.LeftTop(), closeCross.RightBottom());
			StrokeLine(closeCross.RightTop(), closeCross.LeftBottom());

			// Draw the label
			DrawString(label.String(), BPoint(fCollapseRect.right + kEdgePadding, labelOffset + kEdgePadding));
		PopState();
	}

	Sync();	
}


void
AppGroupView::MouseDown(BPoint point)
{
	bool changed = false;
	if (fCloseRect.Contains(point)) {
		changed = true;

		int32 children = fInfo.size();
		for (int32 i = 0; i < children; i++) {
			fInfo[i]->RemoveSelf();
			delete fInfo[i];
		}
		fInfo.clear();
	}

	if (fCollapseRect.Contains(point)) {
		fCollapsed = !fCollapsed;
		changed = true;
	}

	if (changed) {
		ResizeViews();
		Invalidate();
	}
}


void
AppGroupView::GetPreferredSize(float* width, float* height)
{
	font_height fh;
	be_bold_font->GetHeight(&fh);

	float h = fh.ascent + fh.leading + fh.leading;
	h += kEdgePadding * 2; // Padding between top and bottom of label

	if (!fCollapsed) {
		int32 children = fInfo.size();

		for (int32 i = 0; i < children; i++) {
			float childHeight = 0;
			float childWidth = 0;
			
			fInfo[i]->GetPreferredSize(&childWidth, &childHeight);
			
			h += childHeight;
		}
	}

	h += kEdgePadding;

	*width = fParent->ViewWidth();
	*height = h;
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
				view->RemoveSelf();
				delete view;
			}

			ResizeViews();
			Invalidate();

			// When all the views are destroy, save app filters
			if (fInfo.size() == 0)
				dynamic_cast<NotificationWindow*>(Window())->SaveAppFilters();
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
	if (id.Length() > 0) {
		int32 children = fInfo.size();
		bool found = false;

		for (int32 i = 0; i < children; i++) {
			if (fInfo[i]->HasMessageID(id.String())) {
				fInfo[i]->RemoveSelf();
				delete fInfo[i];
				
				fInfo[i] = view;
				found = true;
				
				break;
			}
		}

		if (!found)
			fInfo.push_back(view);
	} else
		fInfo.push_back(view);

	if (fParent->IsHidden())
		fParent->Show();
	if (IsHidden())
		Show();
	if (view->IsHidden())
		view->Show();

	AddChild(view);

	ResizeViews();
	Invalidate();
}


void
AppGroupView::ResizeViews()
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
			fInfo[i]->SetPosition(false, false);
		};
	} else {
		for (int32 i = 0; i < children; i++)
			if (!fInfo[i]->IsHidden())
				fInfo[i]->Hide();
	}

	if (children == 1)
		fInfo[0]->SetPosition(true, true);
	else if (children > 1) {
		fInfo[0]->SetPosition(true, false);
		fInfo[children - 1]->SetPosition(false, true);
	}

	ResizeTo(fParent->ViewWidth(), offset);
	float labelOffset = fh.ascent + fh.leading;

	BRect borderRect = Bounds().InsetByCopy(kEdgePadding, kEdgePadding);
	borderRect.top = 2*labelOffset;

	fCollapseRect = borderRect;
	fCollapseRect.right = fCollapseRect.left + kExpandSize;
	fCollapseRect.bottom = fCollapseRect.top + kExpandSize;
	fCollapseRect.OffsetTo(kEdgePadding * 2, kEdgePadding * 1.5);

	fCloseRect = borderRect;
	fCloseRect.right -= kEdgePadding * 4;
	fCloseRect.left = fCloseRect.right - kCloseSize;
	fCloseRect.bottom = fCloseRect.top + kCloseSize;
	fCloseRect.OffsetTo(fCloseRect.left, kEdgePadding * 1.5);

	fParent->ResizeAll();
}


bool
AppGroupView::HasChildren()
{
	return !fInfo.empty();
}
