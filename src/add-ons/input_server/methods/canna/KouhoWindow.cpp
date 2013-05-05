/*
 * Copyright 2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 1999 M.Kawamura
 */


#include <string.h>

#include <Screen.h>
#ifdef DEBUG
#include <Debug.h>
#endif
#include "WindowPrivate.h"

#include "KouhoWindow.h"


KouhoWindow::KouhoWindow(BFont *font, BLooper *looper)
	:BWindow(	DUMMY_RECT,
				"kouho", B_MODAL_WINDOW_LOOK,
				B_FLOATING_ALL_WINDOW_FEEL,
				B_NOT_RESIZABLE | B_NOT_CLOSABLE |
				B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AVOID_FOCUS |
				B_NOT_ANCHORED_ON_ACTIVATE)
{
	float fontHeight;
	BRect frame;
	BFont indexfont;

	cannaLooper = looper;
	kouhoFont = font;

	font_family family;
	font_style style;
	strcpy(family, "VL PGothic");
	strcpy(style, "regular");
	indexfont.SetFamilyAndStyle(family, style);
	indexfont.SetSize(10);

#ifdef DEBUG
SERIAL_PRINT(("kouhoWindow: Constructor called.\n"));
#endif

	//setup main pane
	indexWidth = indexfont.StringWidth("W") + INDEXVIEW_SIDE_MARGIN * 2;
	minimumWidth = indexfont.StringWidth("ギリシャ  100/100");

	frame = Bounds();
	frame.left = indexWidth + 2;
	frame.bottom -= INFOVIEW_HEIGHT;
	kouhoView = new KouhoView(frame);
	BRect screenrect = BScreen(this).Frame();
	kouhoView->SetTextRect(screenrect); //big enough
	kouhoView->SetFontAndColor(kouhoFont);
	kouhoView->SetWordWrap(false);
	AddChild(kouhoView);
	fontHeight = kouhoView->LineHeight();

	frame = Bounds();
	frame.right = indexWidth;
	frame.bottom = frame.bottom - INFOVIEW_HEIGHT + 1;
	indexView = new KouhoIndexView(frame, fontHeight);
	indexView->SetFont(&indexfont);
	AddChild(indexView);

	frame = Bounds();
	frame.top = frame.bottom - INFOVIEW_HEIGHT + 1;
	infoView = new KouhoInfoView(frame);
	infoView->SetFont(&indexfont);
	infoView->SetAlignment(B_ALIGN_RIGHT);
	AddChild(infoView);
}


void
KouhoWindow::MessageReceived(BMessage* msg)
{
	float height, width, x, y, w;
	BPoint point;
	BRect screenrect, frame;

	switch(msg->what) {
		case KOUHO_WINDOW_HIDE:
#ifdef DEBUG
SERIAL_PRINT(("kouhoWindow: KOUHO_WINDOW_HIDE recieved.\n"));
#endif
			if (!IsHidden()) {
				Hide();
				standalone_mode = false;
			}
			break;

		case KOUHO_WINDOW_SHOW:
#ifdef DEBUG
SERIAL_PRINT(("kouhoWindow: KOUHO_WINDOW_SHOW recieved.\n"));
#endif
			ShowWindow();
			break;

		case KOUHO_WINDOW_SHOW_ALONE:
			standalone_mode = true;
			frame = Frame();
			screenrect = BScreen().Frame();
			frame.OffsetTo(gSettings.standalone_loc.x, gSettings.standalone_loc.y);

			x = screenrect.right - frame.right;
			y = screenrect.bottom - frame.bottom;

			if (x < 0)
				frame.OffsetBy(x, 0);

			if (y < 0)
				frame.OffsetBy(0, y);

			gSettings.standalone_loc.x = frame.left;
			gSettings.standalone_loc.y = frame.top;
			point = frame.LeftTop();
			MoveTo(point);
			ShowWindow();
			break;

		case KOUHO_WINDOW_SHOWAT:
#ifdef DEBUG
SERIAL_PRINT(("kouhoWindow: KOUHO_WINDOW_SHOWAT recieved.\n"));
#endif
			msg->FindPoint("position", &point);
			msg->FindFloat("height", &height);
			ShowAt(point, height);
			break;

		case KOUHO_WINDOW_SETTEXT:
			const char* newtext;
			bool hideindex, limitsize;
			msg->FindString("text", &newtext);
			kouhoView->SetText(newtext);

			msg->FindBool("index", &hideindex);
			indexView->HideNumberDisplay(hideindex);

			msg->FindBool("limit", &limitsize);
			height = kouhoView->TextHeight(0, kouhoView->TextLength());
			height += INFOVIEW_HEIGHT;

			msg->FindString("info", &newtext);
			infoView->SetText(newtext);
			// calculate widest line width
			width = 0;
			for (int32 line = 0, numlines = kouhoView->CountLines();
				line < numlines; line++) {
				w = kouhoView->LineWidth(line);
				if (w > width)
					width = w;
			}
			if (limitsize && width < minimumWidth)
				width = minimumWidth;
			ResizeTo(width + indexWidth + KOUHOVIEW_SIDE_MARGIN, height);

			if (msg->HasBool("partial")) {
				int32 begin, end;
				msg->FindInt32("revbegin", &begin);
				msg->FindInt32("revend", &end);
				kouhoView->HighlightPartial(begin, end);
#ifdef DEBUG
SERIAL_PRINT(("kouhoWindow: KOUHO_WINDOW_SETTEXT(partial) received. rev = %d to %d\n", begin, end));
#endif
			} else {
				int32 kouhorevline;
				msg->FindInt32("kouhorev", &kouhorevline);
				kouhoView->HighlightLine(kouhorevline);
			}

			break;

		case NUM_SELECTED_FROM_KOUHO_WIN:
			cannaLooper->PostMessage(msg);
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}


void
KouhoWindow::ShowAt(BPoint revpoint, float height)
{
	BRect screenrect;
	BPoint point;
	int32 kouhowidth, kouhoheight;

	kouhowidth = Frame().IntegerWidth();
	kouhoheight = Frame().IntegerHeight();

	screenrect = BScreen(this).Frame();
#ifdef DEBUG
SERIAL_PRINT(("KouhoWindow: ShowAt activated. point x= %f, y= %f, height= %f", revpoint.x, revpoint.y, height));
#endif
	//adjust to preferred position - considering window border & index
	//revpoint.y += WINDOW_BORDER_WIDTH;
	point.y = revpoint.y + height + WINDOW_BORDER_WIDTH_V;
	point.x = revpoint.x - indexView->Frame().IntegerWidth()
				- INDEXVIEW_SIDE_MARGIN;

	if (point.y + kouhoheight > screenrect.bottom)
		point.y = revpoint.y - kouhoheight - WINDOW_BORDER_WIDTH_V;
//	else
//		point.y = revpoint.y + height;

	if (point.x + kouhowidth > screenrect.right)
		point.x = point.x - (screenrect.right - (point.x + kouhowidth));
//		point.x = revpoint.x
//			- (revpoint.x + kouhowidth + WINDOW_BORDER_WIDTH - screenrect.right);
//	else
//		point.x = revpoint.x;

	MoveTo(point);
	ShowWindow();
}


void
KouhoWindow::FrameMoved(BPoint screenPoint)
{
	if (standalone_mode)
		gSettings.standalone_loc = screenPoint;
}


// ShowWindow() shows kouho window on current workspace
// This is a bug fix for version 1.0 beta 2
void
KouhoWindow::ShowWindow()
{
	if (IsHidden()) {
		SetWorkspaces(B_CURRENT_WORKSPACE);
		Show();
	}
}


/*
	KouhoView section
*/

KouhoView::KouhoView(BRect rect)
	: BTextView(rect, "kouhoview", rect, B_FOLLOW_ALL, B_WILL_DRAW)
{
	selection_color.red = 203;
	selection_color.green = 152;
	selection_color.blue = 255;
}


void
KouhoView::HighlightLine(int32 line)
{
//printf("highlightLine set to :%d\n", line);
	int32 begin, end;
	BRegion region;

	if (line != -1) {
		begin = OffsetAt(line);
		if (line == CountLines() - 1)
			end = TextLength() + 1;
		else
			end = OffsetAt(line + 1) - 1;
//printf("Highlight line:%d, offset %d-%d\n", line,begin,end);
		GetTextRegion(begin, end, &region);
		//as hightlight is just one line, 0 is enough.
		highlightRect = region.RectAt(0);
		//extend highlihght region to right end
		highlightRect.right = Bounds().right;
		Invalidate();
	}

}


void
KouhoView::HighlightPartial(int32 begin, int32 end)
{
	BRegion region;
	GetTextRegion(begin, end, &region);
	highlightRect = region.RectAt(0);
	Invalidate(highlightRect);
}


void
KouhoView::Draw(BRect rect)
{
	BTextView::Draw(rect);
//	rgb_color viewcolor = ViewColor();
	SetHighColor(selection_color);
	SetDrawingMode(B_OP_MIN);
	FillRect(highlightRect);
//	SetViewColor(viewcolor);
}


void
KouhoView::MouseDown(BPoint point)
{
	KouhoIndexView *iview;
	iview = (KouhoIndexView *)(Window()->FindView("index"));
	if (iview->IsNumberDisplayHidden())
		return;

	int32 number;
	number = LineAt(point);
	BMessage msg(NUM_SELECTED_FROM_KOUHO_WIN);
	msg.AddInt32("modifiers", 0);
	msg.AddInt8("byte", (int8)number + '1');
	Window()->PostMessage(&msg);
}


/*
	KouhoIndexView section
*/

KouhoIndexView::KouhoIndexView(BRect frame, float fontheight)
	: BBox(frame, "index", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW)
{
	font_height ht;
	float indexfontheight;
	lineHeight = fontheight;
	be_plain_font->GetHeight(&ht);
	indexfontheight = ht.ascent + ht.descent + ht.leading;
	if (indexfontheight < lineHeight)
		fontOffset = (int32)((lineHeight - indexfontheight) / 2 + 1.5);
//printf("line height=%f, index font height=%f, offset=%d\n", lineHeight, indexfontheight, fontOffset);

}


void
KouhoIndexView::Draw(BRect rect)
{
	BBox::Draw(rect);
	for (long i = 1; i < 17; i++) {
		MovePenTo(INDEXVIEW_SIDE_MARGIN + 2, lineHeight * i - fontOffset);
		if (hideNumber)
			DrawString("•");
		else
			DrawChar('0' + i);
	}
}

//
// InfoView section
//

KouhoInfoView::KouhoInfoView(BRect frame)
	: BStringView(frame, "infoview", "0/0",
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM)
{
}


void
KouhoInfoView::Draw(BRect rect)
{
	BPoint start, end;
	BRect bounds = Bounds();
	start = B_ORIGIN;
	end.Set(bounds.right, 0);
	StrokeLine(start, end);
	BStringView::Draw(rect);
}

