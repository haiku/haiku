/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "DataView.h"
#include "DataEditor.h"

#include <Looper.h>
#include <ScrollBar.h>


static const uint32 kBlockSize = 16;
static const uint32 kHorizontalSpace = 8;
static const uint32 kVerticalSpace = 4;

static const uint32 kBlockSpace = 3;
static const uint32 kHexByteWidth = 3;
	// these are determined by the implementation of DataView::ConvertLine()


DataView::DataView(BRect rect, DataEditor &editor)
	: BView(rect, "dataView", B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS),
	fEditor(editor),
	fFocus(kHexFocus),
	fBase(kHexBase),
	fIsActive(true)
{
	fPositionLength = 4;
	fStart = fEnd = 0;
	fMouseSelectionStart = -1;

	fDataSize = fEditor.ViewSize();
	fData = (uint8 *)malloc(fDataSize);

	SetFontSize(12.0);
		// also sets the fixed font

	UpdateFromEditor();
}


DataView::~DataView()
{
}


void 
DataView::DetachedFromWindow()
{
	fEditor.StopWatching(this);
}


void 
DataView::AttachedToWindow()
{
	fEditor.StartWatching(this);
}


void 
DataView::UpdateFromEditor(BMessage */*message*/)
{
	if (fData == NULL)
		return;

	if (fEditor.Lock()) {
		fOffset = fEditor.ViewOffset();

		const uint8 *data;
		if (fEditor.GetViewBuffer(&data) == B_OK)
			memcpy(fData, data, fDataSize);

		fEditor.Unlock();
	}
}


void 
DataView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE: {
			int32 what;
			if (message->FindInt32(B_OBSERVE_WHAT_CHANGE, &what) != B_OK)
				break;

			switch (what) {
				case kMsgDataEditorUpdate:
					UpdateFromEditor(message);
					break;
			}
			break;
		}

		case kMsgBaseType:
		{
			int32 type;
			if (message->FindInt32("base", &type) != B_OK)
				break;

			SetBase((base_type)type);
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}


void
DataView::ConvertLine(char *line, off_t offset, const uint8 *buffer, size_t size)
{
	line += sprintf(line, fBase == kHexBase ? "%0*Lx:  " : "%0*Ld:  ",
				(int)fPositionLength, offset);

	for (uint32 i = 0; i < kBlockSize; i++) {
		if (i >= size) {
			strcpy(line, "  ");
			line += 2;
		} else
			line += sprintf(line, "%02x ", *(unsigned char *)(buffer + i));
	}

	strcpy(line, "   ");
	line += 3;

	for (uint32 i = 0; i < kBlockSize; i++) {
		if (i < size) {
			char c = buffer[i];

			if (c < ' ' || c == 0x7f)
				*line++ = '.';
			else
				*line++ = c;
		} else
			break;
	}

	*line = '\0';
}


void 
DataView::Draw(BRect updateRect)
{
	if (fData == NULL)
		return;

	// ToDo: take "updateRect" into account!

	char line[255];
	BPoint location(kHorizontalSpace, kVerticalSpace + fAscent);

	for (uint32 i = 0; i < fDataSize; i += kBlockSize) {
		ConvertLine(line, fOffset + i, fData + i, fDataSize - i);
		DrawString(line, location);

		location.y += fFontHeight;
	}
	// ToDo: clear unused space!

	DrawSelection();
}


int32 
DataView::PositionAt(view_focus focus, BPoint point, view_focus *_newFocus)
{
	if (!Bounds().Contains(point))
		return -1;

	float left = fCharWidth * (fPositionLength + kBlockSpace) + kHorizontalSpace;
	float hexWidth = fCharWidth * kBlockSize * kHexByteWidth;
	float width = fCharWidth;

	if (focus == kNoFocus) {
		// find in which part the point is in
		if (point.x < left - width / 2)
			return -1;

		if (point.x > left + hexWidth)
			focus = kAsciiFocus;
		else
			focus = kHexFocus;

		if (_newFocus)
			*_newFocus = focus;
	}
	if (focus == kHexFocus) {
		left -= width / 2;
		width *= kHexByteWidth;
	} else
		left += hexWidth + (kBlockSpace * width);

	int32 row = int32((point.y - kVerticalSpace) / fFontHeight);
	int32 column = int32((point.x - left) / width);
	if (column >= (int32)kBlockSize)
		column = (int32)kBlockSize -1;
	else if (column < 0)
		column = 0;

	return row * kBlockSize + column;
}


BRect
DataView::SelectionFrame(view_focus which, int32 start, int32 end)
{
	float spacing = 0;
	float width = fCharWidth;
	float byteWidth = fCharWidth;
	float left;

	if (which == kHexFocus) {
		spacing = fCharWidth / 2;
		left = width * (fPositionLength + kBlockSpace);
		width *= kHexByteWidth;
		byteWidth *= 2;
	} else
		left = width * (fPositionLength + 2 * kBlockSpace + kHexByteWidth * kBlockSize);

	left += kHorizontalSpace;
	float startInLine = (start % kBlockSize) * width;
	float endInLine = (end % kBlockSize) * width + byteWidth - 1;

	return BRect(left + startInLine - spacing,
		kVerticalSpace + (start / kBlockSize) * fFontHeight,
		left + endInLine + spacing,
		kVerticalSpace + (end / kBlockSize + 1) * fFontHeight - 1);
}


void
DataView::DrawSelectionFrame(view_focus which)
{
	bool drawBlock = false;
	bool drawLastLine = false;
	BRect block, lastLine;
	int32 spacing = 0;
	if (which == kAsciiFocus)
		spacing++;

	// draw first line

	int32 start = fStart % kBlockSize;
	int32 first = (fStart / kBlockSize) * kBlockSize;

	int32 end = fEnd;
	if (end > first + (int32)kBlockSize - 1)
		end = first + kBlockSize - 1;

	BRect firstLine = SelectionFrame(which, first + start, end);
	firstLine.right += spacing;
	first += kBlockSize;

	// draw block (and last line) if necessary

	end = fEnd % kBlockSize;
	int32 last = (fEnd / kBlockSize) * kBlockSize;

	if (last >= first) {
		if (end == kBlockSize - 1)
			last += kBlockSize;
		if (last > first) {
			block = SelectionFrame(which, first, last - 1);
			block.right += spacing;
			drawBlock = true;
		}
		if (end != kBlockSize - 1) {
			lastLine = SelectionFrame(which, last, last + end);
			lastLine.right += spacing;
			drawLastLine = true;
		}
	}

	SetDrawingMode(B_OP_INVERT);
	BeginLineArray(8);

	// +*******
	// |      *
	// +------+

	const rgb_color color = {0, 0, 0};
	float bottom;
	if (drawBlock)
		bottom = block.bottom;
	else
		bottom = firstLine.bottom;

	AddLine(BPoint(firstLine.left + 1, firstLine.top), firstLine.RightTop(), color);
	AddLine(BPoint(firstLine.right, firstLine.top + 1), BPoint(firstLine.right, bottom), color);

	// *-------+
	// *       |
	// *********

	BRect rect;
	if (start == 0 || !drawBlock && !drawLastLine)
		rect = firstLine;
	else if (drawBlock)
		rect = block;
	else
		rect = lastLine;

	if (drawBlock)
		rect.bottom = block.bottom;
	if (drawLastLine) {
		rect.bottom = lastLine.bottom;
		rect.right = lastLine.right;
	}
	rect.bottom++;

	AddLine(rect.LeftTop(), rect.LeftBottom(), color);
	AddLine(BPoint(rect.left + 1, rect.bottom), rect.RightBottom(), color);

	//     *--------+
	//     *        |
	// +****        |
	// |            |

	if (start && (drawLastLine || drawBlock)) {
		AddLine(firstLine.LeftTop(), firstLine.LeftBottom(), color);

		float right = firstLine.left;
		if (!drawBlock && right > lastLine.right)
			right = lastLine.right;
		AddLine(BPoint(rect.left + 1, rect.top), BPoint(right, rect.top), color);
	}

	// |            |
	// |        *****
	// |        *
	// +--------+

	if (drawLastLine) {
		AddLine(lastLine.RightBottom(), BPoint(lastLine.right, lastLine.top + 1), color);
		if (!drawBlock && lastLine.right <= firstLine.left)
			lastLine.right = firstLine.left + (lastLine.right < firstLine.left ? 0 : 1);
		AddLine(BPoint(lastLine.right, lastLine.top), BPoint(firstLine.right, lastLine.top), color);
	}

	EndLineArray();
	SetDrawingMode(B_OP_COPY);
}


void
DataView::DrawSelectionBlock(view_focus which)
{
	// draw first line

	SetDrawingMode(B_OP_INVERT);

	int32 start = fStart % kBlockSize;
	int32 first = (fStart / kBlockSize) * kBlockSize;

	int32 end = fEnd;
	if (end > first + (int32)kBlockSize - 1)
		end = first + kBlockSize - 1;

	FillRect(SelectionFrame(which, first + start, end));
	first += kBlockSize;

	// draw block (and last line) if necessary

	end = fEnd % kBlockSize;
	int32 last = (fEnd / kBlockSize) * kBlockSize;

	if (last >= first) {
		if (end == kBlockSize - 1)
			last += kBlockSize;

		if (last > first)
			FillRect(SelectionFrame(which, first, last - 1));
		if (end != kBlockSize - 1)
			FillRect(SelectionFrame(which, last, last + end));
	}

	SetDrawingMode(B_OP_COPY);
}


void
DataView::DrawSelection()
{
	if (fIsActive) {
		DrawSelectionBlock(fFocus);
		DrawSelectionFrame(fFocus == kHexFocus ? kAsciiFocus : kHexFocus);
	} else {
		DrawSelectionFrame(kHexFocus);
		DrawSelectionFrame(kAsciiFocus);
	}
}


void
DataView::SetSelection(int32 start, int32 end, view_focus focus)
{
	if (start > end) {
		int32 temp = start;
		start = end;
		end = temp;
	}

	if (start < 0)
		start = 0;
	if (end > (int32)fDataSize - 1)
		end = (int32)fDataSize - 1;

	if (fStart == start && fEnd == end) {
		// nothing has changed, no need to update
		return;
	}

	// remove old selection
	// ToDo: this could be drastically improved if only the changed
	//	parts are drawn! Of course, this would only work for the
	//	selection block, not the frame.
	DrawSelection();

	if (focus != kNoFocus)
		fFocus = focus;
	fStart = start;
	fEnd = end;

	DrawSelection();
}


void
DataView::GetSelection(int32 &start, int32 &end)
{
	start = fStart;
	end = fEnd;
}


void
DataView::InvalidateRange(int32 start, int32 end)
{
	int32 startLine = start / kBlockSize;
	int32 endLine = end / kBlockSize;
	
	if (endLine > startLine) {
		start = startLine * kBlockSize;
		end = (endLine + 1) * kBlockSize - 1;
	}

	// the part with focus
	BRect rect = SelectionFrame(fFocus, start, end);
	rect.bottom++;
	rect.right++;
	Invalidate(rect);

	// the part without focus
	rect = SelectionFrame(fFocus == kHexFocus ? kAsciiFocus : kHexFocus, start, end);
	rect.bottom++;
	rect.right++;
	Invalidate(rect);
}


void 
DataView::SetBase(base_type type)
{
	if (fBase == type)
		return;

	fBase = type;
	Invalidate();
}


void 
DataView::SetFocus(view_focus which)
{
	if (which == fFocus)
		return;

	DrawSelection();
	fFocus = which;
	DrawSelection();
}


void
DataView::WindowActivated(bool active)
{
	fIsActive = active;

	// only redraw the focussed part

	if (fIsActive) {
		DrawSelectionFrame(fFocus);
		DrawSelectionBlock(fFocus);
	} else {
		DrawSelectionBlock(fFocus);
		DrawSelectionFrame(fFocus);
	}
}


void 
DataView::UpdateScroller()
{
	float width, height;
	GetPreferredSize(&width, &height);

	BScrollBar *bar;
	if ((bar = ScrollBar(B_HORIZONTAL)) != NULL) {
		float delta = width - Bounds().Width();
		if (delta < 0)
			delta = 0;

		bar->SetRange(0, delta);
		bar->SetSteps(fCharWidth, Bounds().Width());
		bar->SetProportion(Bounds().Width() / width);
	}
	if ((bar = ScrollBar(B_VERTICAL)) != NULL) {
		float delta = height - Bounds().Height();
		if (delta < 0)
			delta = 0;

		bar->SetRange(0, delta);
		bar->SetSteps(fFontHeight, Bounds().Height());
		bar->SetProportion(Bounds().Height() / height);
	}
}


void
DataView::FrameResized(float width, float height)
{
	UpdateScroller();
}


void 
DataView::MouseDown(BPoint where)
{
	BMessage *message = Looper()->CurrentMessage();
	int32 buttons;
	if (message == NULL || message->FindInt32("buttons", &buttons) != B_OK
		|| (buttons & B_PRIMARY_MOUSE_BUTTON) == 0)
		return;

	int32 modifiers = message->FindInt32("modifiers");

	view_focus newFocus;
	fMouseSelectionStart = PositionAt(kNoFocus, where, &newFocus);
	if (fMouseSelectionStart == -1) {
		// "where" is outside the valid frame
		return;
	}

	int32 selectionEnd = fMouseSelectionStart;
	if (modifiers & B_SHIFT_KEY) {
		// enlarge the current selection
		if (fStart < selectionEnd)
			fMouseSelectionStart = fStart;
		else if (fEnd > selectionEnd)
			fMouseSelectionStart = fEnd;
	}
	SetSelection(fMouseSelectionStart, selectionEnd, newFocus);

	SetMouseEventMask(B_POINTER_EVENTS,
		B_NO_POINTER_HISTORY | B_SUSPEND_VIEW_FOCUS | B_LOCK_WINDOW_FOCUS);
}


void 
DataView::MouseMoved(BPoint where, uint32 transit, const BMessage */*dragMessage*/)
{
	if (fMouseSelectionStart == -1)
		return;

	SetSelection(fMouseSelectionStart, PositionAt(fFocus, where));
}


void 
DataView::MouseUp(BPoint where)
{
	fMouseSelectionStart = -1;
}


void
DataView::SetFont(const BFont *font, uint32 properties)
{
	if (!font->IsFixed())
		return;

	BView::SetFont(font, properties);
	
	font_height fontHeight;
	font->GetHeight(&fontHeight);

	fFontHeight = int32(fontHeight.ascent + fontHeight.descent + fontHeight.leading);
	fAscent = fontHeight.ascent;
	fCharWidth = font->StringWidth("w");
}


void 
DataView::SetFontSize(float point)
{
	BFont font = be_fixed_font;
	font.SetSize(point);

	SetFont(&font);
}


void 
DataView::GetPreferredSize(float *_width, float *_height)
{
	if (_width) {
		BFont font;
		GetFont(&font);
		*_width = fCharWidth * (kBlockSize * 4 + fPositionLength + 6)
			+ 2 * kHorizontalSpace;
	}

	if (_height) {
		*_height = fFontHeight * ((fDataSize + kBlockSize - 1) / kBlockSize)
			 + 2 * kVerticalSpace;
	}
}

