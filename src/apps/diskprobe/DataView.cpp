/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "DataView.h"
#include "DataEditor.h"

#include <stdio.h>
#include <stdlib.h>

#include <Window.h>
#include <ScrollBar.h>
#include <Autolock.h>
#include <Clipboard.h>
#include <Mime.h>
#include <Beep.h>


#ifndef __HAIKU__
typedef uint32 addr_t;
#endif

static const uint32 kBlockSize = 16;
static const uint32 kHorizontalSpace = 8;
static const uint32 kVerticalSpace = 4;

static const uint32 kBlockSpace = 3;
static const uint32 kHexByteWidth = 3;
	// these are determined by the implementation of DataView::ConvertLine()


/*!	This function checks if the buffer contains a valid UTF-8
	string, following the convention from the DataView::ConvertLine()
	method: everything that's not replaced by a '.' will be accepted.
*/
bool
is_valid_utf8(uint8 *data, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		// accept a terminating null byte
		if (i == size - 1 && data[i] == '\0')
			return true;

		if ((data[i] & 0x80) == 0) {
			// a single byte character
			if ((data[i] < ' '
					&& data[i] != 0x0d && data[i] != 0x09 && data[i] != 0x0a)
				|| data[i] == 0x7f)
				return false;

			continue;
		}

		if ((data[i] & 0xc0) == 0x80) {
			// not a proper multibyte start
			return false;
		}

		// start of a multibyte character
		uint8 mask = 0x80;
		uint32 result = (uint32)(data[i++] & 0xff);

		while (result & mask) {
			if (mask == 0x02) {
				// seven byte char - invalid
				return false;
			}

			result &= ~mask;
			mask >>= 1;
		}

		while (i < size && (data[i] & 0xc0) == 0x80) {
			result <<= 6;
			result += data[i++] & 0x3f;

			mask <<= 1;
			if (mask == 0x40)
				break;
		}

		if (mask != 0x40) {
			// not enough bytes in multibyte char
			return false;
		}
	}

	return true;
}


//	#pragma mark -


DataView::DataView(BRect rect, DataEditor &editor)
	: BView(rect, "dataView", B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS),
	fEditor(editor),
	fFocus(kHexFocus),
	fBase(kHexBase),
	fIsActive(true),
	fMouseSelectionStart(-1),
	fKeySelectionStart(-1),
	fBitPosition(0),
	fFitFontSize(false),
	fDragMessageSize(-1)
{
	fPositionLength = 4;
	fStart = fEnd = 0;

	if (fEditor.Lock()) {
		fDataSize = fEditor.ViewSize();
		fOffset = fEditor.ViewOffset();
		fEditor.Unlock();
	} else
		fDataSize = 512;
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
	MakeFocus(true);
		// this seems to be necessary - if we don't do this here,
		// the view is sometimes focus, but IsFocus() returns false...
}


void
DataView::UpdateFromEditor(BMessage *message)
{
	if (fData == NULL)
		return;

	BAutolock locker(fEditor);

	fFileSize = fEditor.FileSize();

	// get the range of the changes

	int32 start = 0, end = fDataSize - 1;
	off_t offset, size;
	if (message != NULL
		&& message->FindInt64("offset", &offset) == B_OK
		&& message->FindInt64("size", &size) == B_OK) {
		if (offset > fOffset + fDataSize
			|| offset + size < fOffset) {
			// the changes are not within our scope, so we can ignore them
			return;
		}

		if (offset > fOffset)
			start = offset - fOffset;
		if (offset + size < fOffset + fDataSize)
			end = offset + size - fOffset;
	}

	if (fOffset + fDataSize > fFileSize)
		fSizeInView = fFileSize - fOffset;
	else
		fSizeInView = fDataSize;

	const uint8 *data;
	if (fEditor.GetViewBuffer(&data) == B_OK)
		// ToDo: copy only the relevant part
		memcpy(fData, data, fDataSize);

	InvalidateRange(start, end);

	// we notify our selection listeners also if the
	// data in the selection has changed
	if (start <= fEnd && end >= fStart) {
		BMessage update;
		update.AddInt64("start", fStart);
		update.AddInt64("end", fEnd);
		SendNotices(kDataViewSelection, &update);
	}
}


bool
DataView::AcceptsDrop(const BMessage *message)
{
	return !fEditor.IsReadOnly()
		&& (message->HasData("text/plain", B_MIME_TYPE)
			|| message->HasData(B_FILE_MIME_TYPE, B_MIME_TYPE));
}


void
DataView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgUpdateData:
		case kMsgDataEditorUpdate:
			UpdateFromEditor(message);
			break;

		case kMsgDataEditorParameterChange:
		{
			int32 viewSize;
			off_t offset;
			if (message->FindInt64("offset", &offset) == B_OK) {
				fOffset = offset;
				SetSelection(0, 0);
				MakeVisible(0);
			}
			if (message->FindInt32("view_size", &viewSize) == B_OK) {
				fDataSize = viewSize;
				fData = (uint8 *)realloc(fData, fDataSize);
				UpdateScroller();
				SendNotices(kDataViewPreferredSize);
			}
			if (message->FindInt64("file_size", &offset) == B_OK)
				UpdateFromEditor();
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

		case kMsgSetSelection:
		{
			int64 start, end;
			if (message->FindInt64("start", &start) != B_OK
				|| message->FindInt64("end", &end) != B_OK)
				break;

			SetSelection(start, end);
			break;
		}

		case B_SELECT_ALL:
			SetSelection(0, fDataSize - 1);
			break;

		case B_COPY:
			Copy();
			break;

		case B_PASTE:
			Paste();
			break;

		case B_UNDO:
			fEditor.Undo();
			break;

		case B_REDO:
			fEditor.Redo();
			break;

		case B_MIME_DATA:
			if (AcceptsDrop(message)) {
				const void *data;
				ssize_t size;
				if (message->FindData("text/plain", B_MIME_TYPE, &data, &size) == B_OK
					|| message->FindData(B_FILE_MIME_TYPE, B_MIME_TYPE, &data, &size) == B_OK) {
					if (fEditor.Replace(fOffset + fStart, (const uint8 *)data, size) != B_OK)
						SetSelection(fStoredStart, fStoredEnd);

					fDragMessageSize = -1;
				}
			}
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
DataView::Copy()
{
	if (!be_clipboard->Lock())
		return;

	be_clipboard->Clear();

	BMessage *clip;
	if ((clip = be_clipboard->Data()) != NULL) {
		uint8 *data = fData + fStart;
		size_t length = fEnd + 1 - fStart;

		clip->AddData(B_FILE_MIME_TYPE, B_MIME_TYPE, data, length);

		if (is_valid_utf8(data, length))
			clip->AddData("text/plain", B_MIME_TYPE, data, length);

		be_clipboard->Commit();
	}

	be_clipboard->Unlock();
}


void
DataView::Paste()
{
	if (!be_clipboard->Lock())
		return;

	const void *data;
	ssize_t length;
	BMessage *clip;
	if ((clip = be_clipboard->Data()) != NULL
		&& (clip->FindData(B_FILE_MIME_TYPE, B_MIME_TYPE, &data, &length) == B_OK
			|| clip->FindData("text/plain", B_MIME_TYPE, &data, &length) == B_OK)) {
		// we have valid data, but it could still be too
		// large to to fit in the file
		if (fOffset + fStart + length > fFileSize)
			length = fFileSize - fOffset;

		if (fEditor.Replace(fOffset + fStart, (const uint8 *)data, length) == B_OK)
			SetSelection(fStart + length, fStart + length);
	} else
		beep();

	be_clipboard->Unlock();
}


void
DataView::ConvertLine(char *line, off_t offset, const uint8 *buffer, size_t size)
{
	if (size == 0) {
		line[0] = '\0';
		return;
	}

	line += sprintf(line, fBase == kHexBase ? "%0*Lx:  " : "%0*Ld:  ",
		(int)fPositionLength, offset);

	for (uint32 i = 0; i < kBlockSize; i++) {
		if (i >= size) {
			strcpy(line, "   ");
			line += kHexByteWidth;
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
	if (fData == NULL || fFileSize == 0)
		return;

	// ToDo: take "updateRect" into account!

	char line[255];
	BPoint location(kHorizontalSpace, kVerticalSpace + fAscent);

	for (uint32 i = 0; i < fSizeInView; i += kBlockSize) {
		ConvertLine(line, i, fData + i, fSizeInView - i);
		DrawString(line, location);

		location.y += fFontHeight;
	}

	DrawSelection();
}


BRect
DataView::DataBounds(bool inView) const
{
	return BRect(0, 0,
		fCharWidth * (kBlockSize * 4 + fPositionLength + 6) + 2 * kHorizontalSpace,
		fFontHeight * (((inView ? fSizeInView : fDataSize) + kBlockSize - 1) / kBlockSize)
			+ 2 * kVerticalSpace);
}


int32
DataView::PositionAt(view_focus focus, BPoint point, view_focus *_newFocus)
{
	// clip the point into our data bounds

	BRect bounds = DataBounds(true);
	if (point.x < bounds.left)
		point.x = bounds.left;
	else if (point.x > bounds.right)
		point.x = bounds.right;

	if (point.y < bounds.top)
		point.y = bounds.top;
	else if (point.y >= bounds.bottom - kVerticalSpace)
		point.y = bounds.bottom - kVerticalSpace - 1;

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
		column = (int32)kBlockSize - 1;
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
	if (fFileSize == 0)
		return;

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
	if (start == 0 || (!drawBlock && !drawLastLine))
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
DataView::DrawSelectionBlock(view_focus which, int32 blockStart, int32 blockEnd)
{
	if (fFileSize == 0)
		return;

	// draw first line

	SetDrawingMode(B_OP_INVERT);

	int32 start = blockStart % kBlockSize;
	int32 first = (blockStart / kBlockSize) * kBlockSize;

	int32 end = blockEnd;
	if (end > first + (int32)kBlockSize - 1)
		end = first + kBlockSize - 1;

	FillRect(SelectionFrame(which, first + start, end));
	first += kBlockSize;

	// draw block (and last line) if necessary

	end = blockEnd % kBlockSize;
	int32 last = (blockEnd / kBlockSize) * kBlockSize;

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
DataView::DrawSelectionBlock(view_focus which)
{
	DrawSelectionBlock(which, fStart, fEnd);
}


void
DataView::DrawSelection(bool frameOnly)
{
	if (IsFocus() && fIsActive) {
		if (!frameOnly)
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
	// correct the values if necessary

	if (start > end) {
		int32 temp = start;
		start = end;
		end = temp;
	}

	if (start > (int32)fSizeInView - 1)
		start = (int32)fSizeInView - 1;
	if (start < 0)
		start = 0;

	if (end > (int32)fSizeInView - 1)
		end = (int32)fSizeInView - 1;
	if (end < 0)
		end = 0;

	if (fStart == start && fEnd == end) {
		// nothing has changed, no need to update
		return;
	}

	// notify our listeners
	if (fStart != start) {
		BMessage update;
		update.AddInt64("position", start);
		SendNotices(kDataViewCursorPosition, &update);
	}

	BMessage update;
	update.AddInt64("start", start);
	update.AddInt64("end", end);
	SendNotices(kDataViewSelection, &update);

	// Update selection - first, we need to remove the old selection, then
	// we redraw the selection with the current values.

	DrawSelection(focus == kNoFocus);
		// From the block selection, only the parts that need updating are
		// actually updated, if there is no focus change.

	if (IsFocus() && fIsActive && focus == kNoFocus) {
		// Update the selection block incrementally

		if (start > fStart) {
			// remove from the top
			DrawSelectionBlock(fFocus, fStart, start - 1);
		} else if (start < fStart) {
			// add to the top
			DrawSelectionBlock(fFocus, start, fStart - 1);
		}

		if (end < fEnd) {
			// remove from bottom
			DrawSelectionBlock(fFocus, end + 1, fEnd);
		} else if (end > fEnd) {
			// add to the bottom
			DrawSelectionBlock(fFocus, fEnd + 1, end);
		}
	}

	if (focus != kNoFocus)
		fFocus = focus;
	fStart = start;
	fEnd = end;

	DrawSelection(focus == kNoFocus);

	fBitPosition = 0;
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
	if (start <= 0 && end >= int32(fDataSize) - 1) {
		Invalidate();
		return;
	}

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
DataView::MakeVisible(int32 position)
{
	if (position < 0 || position > int32(fDataSize) - 1)
		return;

	BRect frame = SelectionFrame(fFocus, position, position);
	BRect bounds = Bounds();
	if (bounds.Contains(frame))
		return;

	// special case the first and the last line and column, so that
	// we can take kHorizontalSpace & kVerticalSpace into account

	if ((position % kBlockSize) == 0)
		frame.left -= kHorizontalSpace;
	else if ((position % kBlockSize) == kBlockSize - 1)
		frame.right += kHorizontalSpace;

	if (position < int32(kBlockSize))
		frame.top -= kVerticalSpace;
	else if (position > int32(fDataSize - kBlockSize))
		frame.bottom += kVerticalSpace;

	// compute the scroll point

	BPoint point = bounds.LeftTop();
	if (bounds.left > frame.left)
		point.x = frame.left;
	else if (bounds.right < frame.right)
		point.x = frame.right - bounds.Width();

	if (bounds.top > frame.top)
		point.y = frame.top;
	else if (bounds.bottom < frame.bottom)
		point.y = frame.bottom - bounds.Height();

	ScrollTo(point);
}


const uint8 *
DataView::DataAt(int32 start)
{
	if (start < 0 || start >= int32(fSizeInView) || fData == NULL)
		return NULL;

	return fData + start;
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
DataView::SetActive(bool active)
{
	if (active == fIsActive)
		return;

	fIsActive = active;

	// only redraw the focussed part

	if (IsFocus() && active) {
		DrawSelectionFrame(fFocus);
		DrawSelectionBlock(fFocus);
	} else {
		DrawSelectionBlock(fFocus);
		DrawSelectionFrame(fFocus);
	}
}


void
DataView::WindowActivated(bool active)
{
	BView::WindowActivated(active);
	SetActive(active);
}


void
DataView::MakeFocus(bool focus)
{
	bool previous = IsFocus();
	BView::MakeFocus(focus);

	if (focus == previous)
		return;

	if (Window()->IsActive() && focus)
		SetActive(true);
	else if (!Window()->IsActive() || !focus)
		SetActive(false);
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
	if (fFitFontSize) {
		// adapt the font size to fit in the view's bounds
		float oldSize = FontSize();
		BFont font = be_fixed_font;
		float steps = 0.5f;

		float size;
		for (size = 1.f; size < 100; size += steps) {
			font.SetSize(size);
			float charWidth = font.StringWidth("w");
			if (charWidth * (kBlockSize * 4 + fPositionLength + 6) + 2 * kHorizontalSpace > width)
				break;

			if (size > 6)
				steps = 1.0f;
		}
		size -= steps;
		font.SetSize(size);

		if (oldSize != size) {
			SetFont(&font);
			Invalidate();
		}
	}

	UpdateScroller();
}


void
DataView::InitiateDrag(view_focus focus)
{
	BMessage *drag = new BMessage(B_MIME_DATA);

	// Add originator and action
	drag->AddPointer("be:originator", this);
	//drag->AddString("be:clip_name", "Byte Clipping");
	//drag->AddInt32("be_actions", B_TRASH_TARGET);

	// Add data (just like in Copy())
	uint8 *data = fData + fStart;
	size_t length = fEnd + 1 - fStart;

	drag->AddData(B_FILE_MIME_TYPE, B_MIME_TYPE, data, length);
	if (is_valid_utf8(data, length))
		drag->AddData("text/plain", B_MIME_TYPE, data, length);

	// get a frame that contains the whole selection - SelectionFrame()
	// only spans a rectangle between the start and the end point, so
	// we have to pass it the correct input values

	BRect frame;
	const int32 width = kBlockSize - 1;
	int32 first = fStart & ~width;
	int32 last = ((fEnd + width) & ~width) - 1;
	if (first == (last & ~width))
		frame = SelectionFrame(focus, fStart, fEnd);
	else
		frame = SelectionFrame(focus, first, last);

	BRect bounds = Bounds();
	if (!bounds.Contains(frame))
		frame = bounds & frame;

	DragMessage(drag, frame, NULL);

	fStoredStart = fStart;
	fStoredEnd = fEnd;
	fDragMessageSize = length;
}


void
DataView::MouseDown(BPoint where)
{
	MakeFocus(true);

	BMessage *message = Looper()->CurrentMessage();
	int32 buttons;
	if (message == NULL || message->FindInt32("buttons", &buttons) != B_OK)
		return;

	view_focus newFocus;
	int32 position = PositionAt(kNoFocus, where, &newFocus);

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0
		&& position >= fStart && position <= fEnd) {
		InitiateDrag(newFocus);
		return;
	}

	if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0)
		return;

	int32 modifiers = message->FindInt32("modifiers");

	fMouseSelectionStart = position;
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
DataView::MouseMoved(BPoint where, uint32 transit, const BMessage *dragMessage)
{
	if (transit == B_EXITED_VIEW && fDragMessageSize > 0) {
		SetSelection(fStoredStart, fStoredEnd);
		fDragMessageSize = -1;
	}

	if (dragMessage && AcceptsDrop(dragMessage)) {
		// handle drag message and tracking

		if (transit == B_ENTERED_VIEW) {
			fStoredStart = fStart;
			fStoredEnd = fEnd;

			const void *data;
			ssize_t size;
			if (dragMessage->FindData("text/plain", B_MIME_TYPE, &data, &size) == B_OK
				|| dragMessage->FindData("text/plain", B_MIME_TYPE, &data, &size) == B_OK)
				fDragMessageSize = size;
		} else if (fDragMessageSize > 0) {
			view_focus newFocus;
			int32 start = PositionAt(kNoFocus, where, &newFocus);
			int32 end = start + fDragMessageSize - 1;

			SetSelection(start, end);
			MakeVisible(start);
		}
		return;
	}

	if (fMouseSelectionStart == -1)
		return;

	int32 end = PositionAt(fFocus, where);
	if (end == -1)
		return;

	SetSelection(fMouseSelectionStart, end);
	MakeVisible(end);
}


void
DataView::MouseUp(BPoint where)
{
	fMouseSelectionStart = fKeySelectionStart = -1;
}


void
DataView::KeyDown(const char *bytes, int32 numBytes)
{
	int32 modifiers;
	if (Looper()->CurrentMessage() == NULL
		|| Looper()->CurrentMessage()->FindInt32("modifiers", &modifiers) != B_OK)
		modifiers = ::modifiers();

	// check if the selection is going to be changed
	switch (bytes[0]) {
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
		case B_UP_ARROW:
		case B_DOWN_ARROW:
			if (modifiers & B_SHIFT_KEY) {
				if (fKeySelectionStart == -1)
					fKeySelectionStart = fStart;
			} else
				fKeySelectionStart = -1;
			break;
	}

	switch (bytes[0]) {
		case B_LEFT_ARROW:
		{
			int32 position = fStart - 1;

			if (modifiers & B_SHIFT_KEY) {
				if (fKeySelectionStart == fEnd)
					SetSelection(fStart - 1, fEnd);
				else {
					SetSelection(fStart, fEnd - 1);
					position = fEnd;
				}
			} else
				SetSelection(fStart - 1, fStart - 1);

			MakeVisible(position);
			break;
		}
		case B_RIGHT_ARROW:
		{
			int32 position = fEnd + 1;

			if (modifiers & B_SHIFT_KEY) {
				if (fKeySelectionStart == fStart)
					SetSelection(fStart, fEnd + 1);
				else
					SetSelection(fStart + 1, fEnd);
			} else
				SetSelection(fEnd + 1, fEnd + 1);

			MakeVisible(position);
			break;
		}
		case B_UP_ARROW:
		{
			int32 start, end;
			if (modifiers & B_SHIFT_KEY) {
				if (fKeySelectionStart == fStart) {
					start = fEnd - int32(kBlockSize);
					end = fStart;
				} else {
					start = fStart - int32(kBlockSize);
					end = fEnd;
				}
				if (start < 0)
					start = 0;
			} else {
				start = fStart - int32(kBlockSize);
				if (start < 0)
					start = fStart;

				end = start;
			}

			SetSelection(start, end);
			MakeVisible(start);
			break;
		}
		case B_DOWN_ARROW:
		{
			int32 start, end;
			if (modifiers & B_SHIFT_KEY) {
				if (fKeySelectionStart == fEnd) {
					start = fEnd;
					end = fStart + int32(kBlockSize);
				} else {
					start = fStart;
					end = fEnd + int32(kBlockSize);
				}
				if (end >= int32(fSizeInView))
					end = int32(fSizeInView) - 1;
			} else {
				end = fEnd + int32(kBlockSize);
				if (end >= int32(fSizeInView))
					start = fEnd;

				start = end;
			}

			SetSelection(start, end);
			MakeVisible(end);
			break;
		}

		case B_PAGE_UP:
		{
			// scroll one page up, but keep the same cursor column

			BRect frame = SelectionFrame(fFocus, fStart, fStart);
			frame.OffsetBy(0, -Bounds().Height());
			if (frame.top <= kVerticalSpace)
				frame.top = kVerticalSpace + 1;
			ScrollBy(0, -Bounds().Height());

			int32 position = PositionAt(fFocus, frame.LeftTop());
			SetSelection(position, position);
			break;
		}
		case B_PAGE_DOWN:
		{
			// scroll one page down, but keep the same cursor column

			BRect frame = SelectionFrame(fFocus, fStart, fStart);
			frame.OffsetBy(0, Bounds().Height());

			float lastLine = DataBounds().Height() - 1 - kVerticalSpace;
			if (frame.top > lastLine)
				frame.top = lastLine;
			ScrollBy(0, Bounds().Height());

			int32 position = PositionAt(fFocus, frame.LeftTop());
			SetSelection(position, position);
			break;
		}
		case B_HOME:
			SetSelection(0, 0);
			MakeVisible(fStart);
			break;
		case B_END:
			SetSelection(fDataSize - 1, fDataSize - 1);
			MakeVisible(fStart);
			break;
		case B_TAB:
			SetFocus(fFocus == kHexFocus ? kAsciiFocus : kHexFocus);
			MakeVisible(fStart);
			break;

		case B_FUNCTION_KEY:
			// this is ignored
			break;

		case B_BACKSPACE:
			if (fBitPosition == 0)
				SetSelection(fStart - 1, fStart - 1);

			if (fFocus == kHexFocus)
				fBitPosition = (fBitPosition + 4) % 8;

			// supposed to fall through
		case B_DELETE:
			SetSelection(fStart, fStart);
				// to make sure only the cursor is selected

			if (fFocus == kHexFocus) {
				const uint8 *data = DataAt(fStart);
				if (data == NULL)
					break;

				uint8 c = data[0] & (fBitPosition == 0 ? 0x0f : 0xf0);
					// mask out region to be cleared

				fEditor.Replace(fOffset + fStart, &c, 1);
			} else
				fEditor.Replace(fOffset + fStart, (const uint8 *)"", 1);
			break;

		default:
			if (fFocus == kHexFocus) {
				// only hexadecimal characters are allowed to be entered
				const uint8 *data = DataAt(fStart);
				uint8 c = bytes[0];
				if (c >= 'A' && c <= 'F')
					c += 'A' - 'a';
				const char *hexNumbers = "0123456789abcdef";
				addr_t number;
				if (data == NULL || (number = (addr_t)strchr(hexNumbers, c)) == 0)
					break;

				SetSelection(fStart, fStart);
					// to make sure only the cursor is selected

				number -= (addr_t)hexNumbers;
				fBitPosition = (fBitPosition + 4) % 8;

				c = (data[0] & (fBitPosition ? 0x0f : 0xf0)) | (number << fBitPosition);
					// mask out overwritten region and bit-wise or the number to be inserted

				if (fEditor.Replace(fOffset + fStart, &c, 1) == B_OK && fBitPosition == 0)
					SetSelection(fStart + 1, fStart + 1);
			} else {
				if (fEditor.Replace(fOffset + fStart, (const uint8 *)bytes, numBytes) == B_OK)
					SetSelection(fStart + 1, fStart + 1);
			}
			break;
	}
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


float
DataView::FontSize() const
{
	BFont font;
	GetFont(&font);

	return font.Size();
}


void
DataView::SetFontSize(float point)
{
	bool fit = (point == 0.0f);
	if (fit) {
		if (!fFitFontSize)
			SendNotices(kDataViewPreferredSize);
		fFitFontSize = fit;

		FrameResized(Bounds().Width(), Bounds().Height());
		return;
	}

	fFitFontSize = false;

	BFont font = be_fixed_font;
	font.SetSize(point);

	SetFont(&font);
	UpdateScroller();
	Invalidate();

	SendNotices(kDataViewPreferredSize);
}


void
DataView::GetPreferredSize(float *_width, float *_height)
{
	BRect bounds = DataBounds();

	if (_width)
		*_width = bounds.Width();

	if (_height)
		*_height = bounds.Height();
}

