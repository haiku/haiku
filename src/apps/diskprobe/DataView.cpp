/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "DataView.h"
#include "DataEditor.h"

#include <Messenger.h>


static const uint32 kBlockSize = 16;
static const uint32 kHorizontalSpace = 8;
static const uint32 kVerticalSpace = 4;


DataView::DataView(BRect rect, DataEditor &editor)
	: BView(rect, "dataView", B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS),
	fEditor(editor)
{
	fPositionLength = 4;
	fDataSize = fEditor.ViewSize();
	fData = (uint8 *)malloc(fDataSize);

	SetFontSize(12.0);

	UpdateFromEditor();

	SetFont(be_fixed_font);
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

		default:
			BView::MessageReceived(message);
	}
}


void
DataView::ConvertLine(char *line, off_t offset, const uint8 *buffer, size_t size)
{
	line += sprintf(line, "%0*Lx:  ", (int)fPositionLength, offset);

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
		*_width = font.StringWidth("w") * (kBlockSize * 4 + fPositionLength + 6)
			+ 2 * kHorizontalSpace;
	}

	if (_height) {
		*_height = fFontHeight * ((fDataSize + kBlockSize - 1) / kBlockSize)
			 + 2 * kVerticalSpace;
	}
}

