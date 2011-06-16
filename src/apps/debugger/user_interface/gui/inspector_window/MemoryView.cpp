/*
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MemoryView.h"

#include <algorithm>

#include <stdio.h>

#include <Messenger.h>
#include <ScrollView.h>
#include <String.h>

#include "TeamMemoryBlock.h"


enum {
	MSG_TARGET_ADDRESS_CHANGED = 'mtac'
};


MemoryView::MemoryView()
	:
	BView("memoryView", B_WILL_DRAW | B_FRAME_EVENTS | B_SUBPIXEL_PRECISE),
	fTargetBlock(NULL),
	fTargetAddress(0LL),
	fCharWidth(0.0),
	fLineHeight(0.0),
	fTextCharsPerLine(0),
	fHexBlocksPerLine(0),
	fHexMode(HexMode8BitInt),
	fTextMode(TextModeASCII)
{
}


MemoryView::~MemoryView()
{
	if (fTargetBlock != NULL)
		fTargetBlock->ReleaseReference();
}


/*static */ MemoryView*
MemoryView::Create()
{
	MemoryView* self = new MemoryView();

	try {
		self->_Init();
	} catch(...) {
		delete self;
		throw;
	}

	return self;
}


void
MemoryView::SetTargetAddress(TeamMemoryBlock* block, target_addr_t address)
{
	fTargetAddress = address;
	if (fTargetBlock != NULL)
		fTargetBlock->ReleaseReference();

	fTargetBlock = block;
	fTargetBlock->AcquireReference();
	BMessenger(this).SendMessage(MSG_TARGET_ADDRESS_CHANGED);
}


void
MemoryView::TargetedByScrollView(BScrollView* scrollView)
{
	BView::TargetedByScrollView(scrollView);
	scrollView->ScrollBar(B_VERTICAL)->SetRange(0.0, 0.0);
}


void
MemoryView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
	SetFont(be_fixed_font);
	fCharWidth = be_fixed_font->StringWidth("a");
	font_height fontHeight;
	be_fixed_font->GetHeight(&fontHeight);
	fLineHeight = fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading;
}


void
MemoryView::Draw(BRect rect)
{
	BView::Draw(rect);

	float divider = 9 * fCharWidth;
	StrokeLine(BPoint(divider, rect.top),
				BPoint(divider, rect.bottom));

	if (fTargetBlock == NULL)
		return;

	uint32 hexBlockSize = 1 << fHexMode;
	if (fHexMode != HexModeNone && fTextMode != TextModeNone) {
		divider += (fHexBlocksPerLine * (hexBlockSize + 1) + 1) * fCharWidth;
		StrokeLine(BPoint(divider, rect.top),
					BPoint(divider, rect.bottom));
	}

	char buffer[32];
	char textbuffer[512];

	int32 startLine = int32(rect.top / fLineHeight) - 1;
	if (startLine < 0)
		startLine = 0;
	int32 endLine = int32(rect.bottom / fLineHeight) + 1;
	int32 startByte = fHexBlocksPerLine * hexBlockSize * startLine;
	const char* currentAddress = (const char*)(fTargetBlock->BaseAddress()
		+ startByte);
	const char* maxAddress = (const char*)(fTargetBlock->BaseAddress()
		+ fTargetBlock->Size());
	BPoint drawPoint(1.0, (startLine + 1) * fLineHeight);
	int32 currentBlocksPerLine = fHexBlocksPerLine;
	int32 currentCharsPerLine = fTextCharsPerLine;
	rgb_color addressColor = tint_color(HighColor(), B_LIGHTEN_1_TINT);
	rgb_color dataColor = HighColor();
	for (int32 i = startLine; i <= endLine && currentAddress < maxAddress;
		i++, drawPoint.y += fLineHeight) {
		drawPoint.x = 1.0;
		snprintf(buffer, sizeof(buffer), "%" B_PRIx32,
			(uint32)currentAddress);
		SetHighColor(addressColor);
		DrawString(buffer, drawPoint);
		drawPoint.x += fCharWidth * 10;
		SetHighColor(dataColor);

		if (fHexMode != HexModeNone) {
			if (currentAddress + (currentBlocksPerLine * hexBlockSize)
				> maxAddress) {
				currentCharsPerLine = maxAddress - currentAddress;
				currentBlocksPerLine = currentCharsPerLine
					/ hexBlockSize;
			}

			for (int32 j = 0; j < currentBlocksPerLine; j++) {
				_GetNextHexBlock(buffer,
					std::min(hexBlockSize + 1, sizeof(buffer)),
					currentAddress + (j * hexBlockSize / 2));
				DrawString(buffer, drawPoint);
				drawPoint.x += fCharWidth * (hexBlockSize + 1);
			}
		}
		if (fTextMode != TextModeNone) {
			drawPoint.x += fCharWidth;
			for (int32 j = 0; j < currentCharsPerLine; j++) {
				// filter non-printable characters
				textbuffer[j] = currentAddress[j] > 32 ? currentAddress[j]
					: '.';
			}
			textbuffer[fTextCharsPerLine] = '\0';
			DrawString(textbuffer, drawPoint);
		}
		if (currentBlocksPerLine > 0)
			currentAddress += currentBlocksPerLine * hexBlockSize / 2;
		else
			currentAddress += fTextCharsPerLine;
	}
}


void
MemoryView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	_RecalcScrollBars();
	Invalidate();
}


void
MemoryView::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case MSG_TARGET_ADDRESS_CHANGED:
		{
			_RecalcScrollBars();
			Invalidate();
			break;
		}
		case MSG_SET_HEX_MODE:
		{
			int32 mode;
			if (message->FindInt32("mode", &mode) == B_OK) {
				fHexMode = mode;
				_RecalcScrollBars();
				Invalidate();
			}
			break;
		}
		case MSG_SET_TEXT_MODE:
		{
			int32 mode;
			if (message->FindInt32("mode", &mode) == B_OK) {
				fTextMode = mode;
				_RecalcScrollBars();
				Invalidate();
			}
			break;
		}
		default:
		{
			BView::MessageReceived(message);
			break;
		}
	}
}


void
MemoryView::_Init()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
MemoryView::_RecalcScrollBars()
{
	float max = 0.0;
	BScrollBar *scrollBar = ScrollBar(B_VERTICAL);
	if (fTargetBlock != NULL) {
		BRect bounds = Bounds();
		// the left portion of the view is off limits since it
		// houses the address offset of the current line
		float baseWidth = bounds.Width() - (10.0 * fCharWidth);
		float hexWidth = 0.0;
		float textWidth = 0.0;
		int32 hexDigits = 1 << fHexMode;
		int32 sizeFactor = 1 + hexDigits;
		if (fHexMode != HexModeNone) {
			if (fTextMode != TextModeNone) {
				float hexProportion = sizeFactor / (float)(sizeFactor
					+ hexDigits / 2);
				hexWidth = baseWidth * hexProportion;
				// when sharing the display between hex and text,
				// we allocate a 2 character space to separate the views
				hexWidth -= 2 * fCharWidth;
				textWidth = baseWidth - hexWidth;
			} else
				hexWidth = baseWidth;
		} else if (fTextMode != TextModeNone)
			textWidth = baseWidth;

		int32 nybblesPerLine = int32(hexWidth / fCharWidth);
		fHexBlocksPerLine = 0;
		fTextCharsPerLine = 0;
		if (fHexMode != HexModeNone) {
			fHexBlocksPerLine = nybblesPerLine / sizeFactor;
			fHexBlocksPerLine &= ~1;
			if (fTextMode != TextModeNone)
				fTextCharsPerLine = fHexBlocksPerLine * (hexDigits / 2);
		} else if (fTextMode != TextModeNone)
			fTextCharsPerLine = int32(textWidth / fCharWidth);

		int32 lineCount = 0;
		float totalHeight = 0.0;
		if (fHexBlocksPerLine > 0) {
			lineCount = fTargetBlock->Size() / (fHexBlocksPerLine
					* hexDigits);
		} else if (fTextCharsPerLine > 0)
			lineCount = fTargetBlock->Size() / fTextCharsPerLine;

		totalHeight = lineCount * fLineHeight;
		if (totalHeight > 0.0) {
			max = totalHeight - bounds.Height();
			scrollBar->SetProportion(bounds.Height() / totalHeight);
			scrollBar->SetSteps(fLineHeight, bounds.Height());
		}
	}
	scrollBar->SetRange(0.0, max);
}

void
MemoryView::_GetNextHexBlock(char* buffer, int32 bufferSize,
	const char* address)
{
	switch(fHexMode) {
		case HexMode8BitInt:
		{
			snprintf(buffer, bufferSize, "%02" B_PRIx8, *address);
			break;
		}
		case HexMode16BitInt:
		{
			snprintf(buffer, bufferSize, "%04" B_PRIx16,
				*((const uint16*)address));
			break;
		}
		case HexMode32BitInt:
		{
			snprintf(buffer, bufferSize, "%08" B_PRIx32,
				*((const uint32*)address));
			break;
		}
		case HexMode64BitInt:
		{
			snprintf(buffer, bufferSize, "%0*" B_PRIx64,
				16, *((const uint64*)address));
			break;
		}
	}
}
