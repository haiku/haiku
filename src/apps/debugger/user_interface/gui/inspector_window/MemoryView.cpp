/*
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MemoryView.h"

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
	fTargetAddress(0)
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

	StrokeLine(BPoint(9 * fCharWidth, rect.top),
				BPoint(9 * fCharWidth, rect.bottom));

	if (fTargetBlock == NULL)
		return;

	char buffer[32];

	int32 startLine = (int32)rect.top / fLineHeight;
	int32 bytesPerLine = fNybblesPerLine / 2;
	int32 startByte = bytesPerLine * startLine;
	target_addr_t currentAddress = fTargetBlock->BaseAddress() + startByte;
	target_addr_t maxAddress = fTargetBlock->BaseAddress()
		+ fTargetBlock->Size();
	BPoint drawPoint(1.0, rect.top);
	int32 currentBytesPerLine = bytesPerLine;

	for (int32 i = startLine; drawPoint.y < rect.bottom
		&& currentAddress < maxAddress; i++, drawPoint.y += fLineHeight) {
		drawPoint.x = 1.0;
		snprintf(buffer, sizeof(buffer), "%" B_PRIx32 "  ",
			(uint32)currentAddress);
		DrawString(buffer, drawPoint);
		drawPoint.x += fCharWidth * 10;
		if (currentAddress + bytesPerLine > maxAddress)
			currentBytesPerLine = maxAddress - currentAddress;
		for (int32 j = 0; j < currentBytesPerLine; j += 2) {
			snprintf(buffer, sizeof(buffer), "%04" B_PRIx16 " ",
				*((uint16*)&fTargetBlock->Data()[i * bytesPerLine + j]));
			DrawString(buffer, drawPoint);
			drawPoint.x += fCharWidth * 5;
		}
		currentAddress += bytesPerLine;
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
		fNybblesPerLine = bounds.Width() / fCharWidth;
		// we allocate 8 characters for the starting address of the current
		// line plus some spacing to separate that from the data
		fNybblesPerLine -= 10;
		// also allocate a space between each 16-bit grouping
		fNybblesPerLine -= (fNybblesPerLine / 4);
		fNybblesPerLine &= ~3;
		int32 lineCount = ceil(2 * fTargetBlock->Size() / fNybblesPerLine);
		float totalHeight = lineCount * fLineHeight;
		max = totalHeight - bounds.Height();
		scrollBar->SetProportion(bounds.Height() / totalHeight);
		scrollBar->SetSteps(fLineHeight, bounds.Height());
	}
	scrollBar->SetRange(0.0, max);
}
