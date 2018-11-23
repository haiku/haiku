/*
 * Copyright 2011-2015, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MemoryView.h"

#include <algorithm>

#include <ctype.h>
#include <stdio.h>

#include <ByteOrder.h>
#include <Clipboard.h>
#include <Looper.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <ScrollView.h>
#include <String.h>

#include "Architecture.h"
#include "AutoDeleter.h"
#include "MessageCodes.h"
#include "Team.h"
#include "TeamMemoryBlock.h"


enum {
	MSG_TARGET_ADDRESS_CHANGED	= 'mtac',
	MSG_VIEW_AUTOSCROLL			= 'mvas'
};

static const bigtime_t kScrollTimer = 10000LL;


MemoryView::MemoryView(::Team* team, Listener* listener)
	:
	BView("memoryView", B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE
		| B_SUBPIXEL_PRECISE),
	fTeam(team),
	fTargetBlock(NULL),
	fEditableData(NULL),
	fEditedOffsets(),
	fTargetAddress(0LL),
	fEditMode(false),
	fEditLowNybble(false),
	fCharWidth(0.0),
	fLineHeight(0.0),
	fTextCharsPerLine(0),
	fHexBlocksPerLine(0),
	fHexMode(HexMode8BitInt),
	fTextMode(TextModeASCII),
	fSelectionBase(0),
	fSelectionStart(0),
	fSelectionEnd(0),
	fScrollRunner(NULL),
	fTrackingMouse(false),
	fListener(listener)
{
	Architecture* architecture = team->GetArchitecture();
	fTargetAddressSize = architecture->AddressSize() * 2;
	fCurrentEndianMode = architecture->IsBigEndian()
		? EndianModeBigEndian : EndianModeLittleEndian;

}


MemoryView::~MemoryView()
{
	if (fTargetBlock != NULL)
		fTargetBlock->ReleaseReference();

	delete[] fEditableData;
}


/*static */ MemoryView*
MemoryView::Create(::Team* team, Listener* listener)
{
	MemoryView* self = new MemoryView(team, listener);

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
	if (block != fTargetBlock) {
		if (fTargetBlock != NULL)
			fTargetBlock->ReleaseReference();

		fTargetBlock = block;
		if (block != NULL)
			fTargetBlock->AcquireReference();
	}

	MakeFocus(true);
	BMessenger(this).SendMessage(MSG_TARGET_ADDRESS_CHANGED);
}


void
MemoryView::UnsetListener()
{
	fListener = NULL;
}


status_t
MemoryView::SetEditMode(bool enabled)
{
	if (fTargetBlock == NULL)
		return B_BAD_VALUE;
	else if (fEditMode == enabled)
		return B_OK;

	if (enabled) {
		status_t error = _SetupEditableData();
		if (error != B_OK)
			return error;
	} else {
		delete[] fEditableData;
		fEditableData = NULL;
		fEditedOffsets.clear();
		fEditLowNybble = false;
	}

	fEditMode = enabled;
	Invalidate();

	return B_OK;
}


void
MemoryView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	SetFont(be_fixed_font);
	fCharWidth = be_fixed_font->StringWidth("a");
	font_height fontHeight;
	be_fixed_font->GetHeight(&fontHeight);
	fLineHeight = ceilf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading);
}


void
MemoryView::Draw(BRect rect)
{
	rect = Bounds();

	float divider = (fTargetAddressSize + 1) * fCharWidth;
	StrokeLine(BPoint(divider, rect.top),
				BPoint(divider, rect.bottom));

	if (fTargetBlock == NULL)
		return;

	uint32 hexBlockSize = _GetHexDigitsPerBlock() + 1;
	uint32 blockByteSize = hexBlockSize / 2;
	if (fHexMode != HexModeNone && fTextMode != TextModeNone) {
		divider += (fHexBlocksPerLine * hexBlockSize + 1) * fCharWidth;
		StrokeLine(BPoint(divider, rect.top),
					BPoint(divider, rect.bottom));
	}

	char buffer[32];
	char textbuffer[512];

	const char* dataSource = (const char*)(fEditMode ? fEditableData
			: fTargetBlock->Data());

	int32 startLine = int32(rect.top / fLineHeight);
	const char* currentAddress = dataSource + fHexBlocksPerLine
		* blockByteSize * startLine;
	const char* maxAddress = dataSource + fTargetBlock->Size();
	const char* targetAddress = dataSource + fTargetAddress
		- fTargetBlock->BaseAddress();
	BPoint drawPoint(1.0, (startLine + 1) * fLineHeight);
	int32 currentBlocksPerLine = fHexBlocksPerLine;
	int32 currentCharsPerLine = fTextCharsPerLine;
	font_height fh;
	GetFontHeight(&fh);
	target_addr_t lineAddress = fTargetBlock->BaseAddress() + startLine
		* currentCharsPerLine;
	bool highlightBlock = false;
	rgb_color highlightColor;
	for (; currentAddress < maxAddress && drawPoint.y < rect.bottom
		+ fLineHeight; drawPoint.y += fLineHeight) {
		drawPoint.x = 1.0;
		snprintf(buffer, sizeof(buffer), "%0*" B_PRIx64,
			(int)fTargetAddressSize, lineAddress);
		PushState();
		SetHighColor(tint_color(HighColor(), B_LIGHTEN_1_TINT));
		DrawString(buffer, drawPoint);
		drawPoint.x += fCharWidth * (fTargetAddressSize + 2);
		PopState();
		if (fHexMode != HexModeNone) {
			if (currentAddress + (currentBlocksPerLine * blockByteSize)
				> maxAddress) {
				currentCharsPerLine = maxAddress - currentAddress;
				currentBlocksPerLine = currentCharsPerLine
					/ blockByteSize;
			}

			for (int32 j = 0; j < currentBlocksPerLine; j++) {
				const char* blockAddress = currentAddress + (j
					* blockByteSize);
				_GetNextHexBlock(buffer, sizeof(buffer), blockAddress);

				highlightBlock = false;
				if (fEditMode)
				{
					int32 offset = blockAddress - dataSource;
					for (uint32 i = 0; i < blockByteSize; i++) {
						if (fEditedOffsets.count(offset + i) != 0) {
							highlightBlock = true;
							highlightColor.set_to(0, 216, 0);
							break;
						}
					}

				} else if (targetAddress >= blockAddress && targetAddress <
						blockAddress + blockByteSize) {
						highlightBlock = true;
						highlightColor.set_to(216, 0, 0);
				}

				if (highlightBlock) {
					PushState();
					SetHighColor(highlightColor);
				}

				DrawString(buffer, drawPoint);

				if (highlightBlock)
					PopState();

				drawPoint.x += fCharWidth * hexBlockSize;
			}

			if (currentBlocksPerLine < fHexBlocksPerLine)
				drawPoint.x += fCharWidth * hexBlockSize
					* (fHexBlocksPerLine - currentBlocksPerLine);
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
			if (targetAddress >= currentAddress && targetAddress
				< currentAddress + currentCharsPerLine) {
				PushState();
				SetHighColor(B_TRANSPARENT_COLOR);
				SetDrawingMode(B_OP_INVERT);
				uint32 blockAddress = uint32(targetAddress - currentAddress);
				if (fHexMode != HexModeNone)
					blockAddress &= ~(blockByteSize - 1);
				float startX = drawPoint.x + fCharWidth * blockAddress;
				float endX = startX;
				if (fHexMode != HexModeNone)
					endX += fCharWidth * ((hexBlockSize - 1) / 2);
				else
					endX += fCharWidth;
				FillRect(BRect(startX, drawPoint.y - fh.ascent, endX,
					drawPoint.y + fh.descent));
				PopState();
			}
		}
		if (currentBlocksPerLine > 0) {
			currentAddress += currentBlocksPerLine * blockByteSize;
			lineAddress += currentBlocksPerLine * blockByteSize;
		} else {
			currentAddress += fTextCharsPerLine;
			lineAddress += fTextCharsPerLine;
		}
	}

	if (fSelectionStart != fSelectionEnd) {
		PushState();
		BRegion selectionRegion;
		_GetSelectionRegion(selectionRegion);
		SetDrawingMode(B_OP_INVERT);
		FillRegion(&selectionRegion, B_SOLID_HIGH);
		PopState();
	}

	if (fEditMode) {
		PushState();
		BRect caretRect;
		_GetEditCaretRect(caretRect);
		SetDrawingMode(B_OP_INVERT);
		FillRect(caretRect, B_SOLID_HIGH);
		PopState();

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
MemoryView::KeyDown(const char* bytes, int32 numBytes)
{
	bool handled = true;
	if (fTargetBlock != NULL) {
		target_addr_t newAddress = fTargetAddress;
		target_addr_t maxAddress = fTargetBlock->BaseAddress()
			+ fTargetBlock->Size() - 1;
		int32 blockSize = 1;
		if (fHexMode != HexModeNone)
			blockSize = 1 << (fHexMode - 1);
		int32 lineCount = int32(Bounds().Height() / fLineHeight);

		switch(bytes[0]) {
			case B_UP_ARROW:
			{
				newAddress -= blockSize * fHexBlocksPerLine;
				break;
			}
			case B_DOWN_ARROW:
			{
				newAddress += blockSize * fHexBlocksPerLine;
				break;
			}
			case B_LEFT_ARROW:
			{
				if (fEditMode) {
					if (!fEditLowNybble)
						newAddress--;
					fEditLowNybble = !fEditLowNybble;
					if (newAddress == fTargetAddress)
						Invalidate();
				} else
					newAddress -= blockSize;
				break;
			}
			case B_RIGHT_ARROW:
			{
				if (fEditMode) {
					if (fEditLowNybble)
						newAddress++;
					fEditLowNybble = !fEditLowNybble;
					if (newAddress == fTargetAddress)
						Invalidate();
				} else
					newAddress += blockSize;
				break;
			}
			case B_PAGE_UP:
			{
				newAddress -= (blockSize * fHexBlocksPerLine) * lineCount;
				break;
			}
			case B_PAGE_DOWN:
			{
				newAddress += (blockSize * fHexBlocksPerLine) * lineCount;
				break;
			}
			case B_HOME:
			{
				newAddress = fTargetBlock->BaseAddress();
				fEditLowNybble = false;
				break;
			}
			case B_END:
			{
				newAddress = maxAddress;
				fEditLowNybble = true;
				break;
			}
			default:
			{
				if (fEditMode && isxdigit(bytes[0]))
				{
					int value = 0;
					if (isdigit(bytes[0]))
						value = bytes[0] - '0';
					else
						value = (int)strtol(bytes, NULL, 16);

					int32 byteOffset = fTargetAddress
						- fTargetBlock->BaseAddress();

					if (fEditLowNybble)
						value = (fEditableData[byteOffset] & 0xf0) | value;
					else {
						value = (fEditableData[byteOffset] & 0x0f)
							| (value << 4);
					}

					fEditableData[byteOffset] = value;

					if (fEditableData[byteOffset]
						!= fTargetBlock->Data()[byteOffset]) {
						fEditedOffsets.insert(byteOffset);
					} else
						fEditedOffsets.erase(byteOffset);

					if (fEditLowNybble) {
						if (newAddress < maxAddress) {
							newAddress++;
							fEditLowNybble = false;
						}
					} else
						fEditLowNybble = true;

					Invalidate();
				} else
					handled = false;

				break;
			}
		}

		if (handled) {
			if (newAddress < fTargetBlock->BaseAddress())
				newAddress = fTargetAddress;
			else if (newAddress > maxAddress)
				newAddress = maxAddress;

			if (newAddress != fTargetAddress) {
				fTargetAddress = newAddress;
				BMessenger(this).SendMessage(MSG_TARGET_ADDRESS_CHANGED);
			}
		}
	} else
		handled = false;

	if (!handled)
		BView::KeyDown(bytes, numBytes);
}


void
MemoryView::MakeFocus(bool isFocused)
{
	BScrollView* parent = dynamic_cast<BScrollView*>(Parent());
	if (parent != NULL)
		parent->SetBorderHighlighted(isFocused);

	BView::MakeFocus(isFocused);
}


void
MemoryView::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case B_COPY:
		{
			_CopySelectionToClipboard();
			break;
		}
		case MSG_TARGET_ADDRESS_CHANGED:
		{
			_RecalcScrollBars();
			ScrollToSelection();
			Invalidate();
			if (fListener != NULL)
				fListener->TargetAddressChanged(fTargetAddress);
			break;
		}
		case MSG_SET_HEX_MODE:
		{
			// while editing, hex view changes are disallowed.
			if (fEditMode)
				break;

			int32 mode;
			if (message->FindInt32("mode", &mode) == B_OK) {
				if (fHexMode == mode)
					break;

				fHexMode = mode;
				_RecalcScrollBars();
				Invalidate();

				if (fListener != NULL)
					fListener->HexModeChanged(mode);
			}
			break;
		}
		case MSG_SET_ENDIAN_MODE:
		{
			int32 mode;
			if (message->FindInt32("mode", &mode) == B_OK) {
				if (fCurrentEndianMode == mode)
					break;

				fCurrentEndianMode = mode;
				Invalidate();

				if (fListener != NULL)
					fListener->EndianModeChanged(mode);
			}
			break;
		}
		case MSG_SET_TEXT_MODE:
		{
			int32 mode;
			if (message->FindInt32("mode", &mode) == B_OK) {
				if (fTextMode == mode)
					break;

				fTextMode = mode;
				_RecalcScrollBars();
				Invalidate();

				if (fListener != NULL)
					fListener->TextModeChanged(mode);
			}
			break;
		}
		case MSG_VIEW_AUTOSCROLL:
		{
			_HandleAutoScroll();
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
MemoryView::MouseDown(BPoint point)
{
	if (!IsFocus())
		MakeFocus(true);

	if (fTargetBlock == NULL)
		return;

	int32 buttons;
	if (Looper()->CurrentMessage()->FindInt32("buttons", &buttons) != B_OK)
		buttons = B_PRIMARY_MOUSE_BUTTON;

	if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		_HandleContextMenu(point);
		return;
	}

	int32 offset = _GetOffsetAt(point);
	if (offset < fSelectionStart || offset > fSelectionEnd) {
		BRegion oldSelectionRegion;
		_GetSelectionRegion(oldSelectionRegion);
		fSelectionBase = offset;
		fSelectionStart = fSelectionBase;
		fSelectionEnd = fSelectionBase;
		Invalidate(oldSelectionRegion.Frame());
	}

	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	fTrackingMouse = true;
}


void
MemoryView::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if (!fTrackingMouse)
		return;

	BRegion oldSelectionRegion;
	_GetSelectionRegion(oldSelectionRegion);
	int32 offset = _GetOffsetAt(point);
	if (offset < fSelectionBase) {
		fSelectionStart = offset;
		fSelectionEnd = fSelectionBase;
	} else {
		fSelectionStart = fSelectionBase;
		fSelectionEnd = offset;
	}

	BRegion region;
	_GetSelectionRegion(region);
	region.Include(&oldSelectionRegion);
	Invalidate(region.Frame());

	switch (transit) {
		case B_EXITED_VIEW:
			fScrollRunner = new BMessageRunner(BMessenger(this),
				new BMessage(MSG_VIEW_AUTOSCROLL), kScrollTimer);
			break;

		case B_ENTERED_VIEW:
			delete fScrollRunner;
			fScrollRunner = NULL;
			break;

		default:
			break;
	}
}


void
MemoryView::MouseUp(BPoint point)
{
	fTrackingMouse = false;
	delete fScrollRunner;
	fScrollRunner = NULL;
}


void
MemoryView::ScrollToSelection()
{
	if (fTargetBlock != NULL) {
		target_addr_t offset = fTargetAddress - fTargetBlock->BaseAddress();
		int32 lineNumber = 0;
		if (fHexBlocksPerLine > 0) {
			lineNumber = offset / (fHexBlocksPerLine
				* (_GetHexDigitsPerBlock() / 2));
		} else if (fTextCharsPerLine > 0)
			lineNumber = offset / fTextCharsPerLine;

		float y = lineNumber * fLineHeight;
		if (y < Bounds().top)
			ScrollTo(0.0, y);
		else if (y + fLineHeight > Bounds().bottom)
			ScrollTo(0.0, y + fLineHeight - Bounds().Height());
	}
}


void
MemoryView::TargetedByScrollView(BScrollView* scrollView)
{
	BView::TargetedByScrollView(scrollView);
	scrollView->ScrollBar(B_VERTICAL)->SetRange(0.0, 0.0);
}


BSize
MemoryView::MinSize()
{
	return BSize(0.0, 0.0);
}


BSize
MemoryView::PreferredSize()
{
	return MinSize();
}


BSize
MemoryView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


void
MemoryView::_Init()
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}


void
MemoryView::_RecalcScrollBars()
{
	float max = 0.0;
	BScrollBar *scrollBar = ScrollBar(B_VERTICAL);
	if (fTargetBlock != NULL) {
		int32 hexDigits = _GetHexDigitsPerBlock();
		int32 sizeFactor = 1 + hexDigits;
		_RecalcBounds();

		float hexWidth = fHexRight - fHexLeft;
		int32 nybblesPerLine = int32(hexWidth / fCharWidth);
		fHexBlocksPerLine = 0;
		fTextCharsPerLine = 0;
		if (fHexMode != HexModeNone) {
			fHexBlocksPerLine = nybblesPerLine / sizeFactor;
			fHexBlocksPerLine &= ~1;
			fHexRight = fHexLeft + (fHexBlocksPerLine * sizeFactor
				* fCharWidth);
			if (fTextMode != TextModeNone)
				fTextCharsPerLine = fHexBlocksPerLine * hexDigits / 2;
		} else if (fTextMode != TextModeNone)
			fTextCharsPerLine = int32((fTextRight - fTextLeft) / fCharWidth);
		int32 lineCount = 0;
		float totalHeight = 0.0;
		if (fHexBlocksPerLine > 0) {
			lineCount = fTargetBlock->Size() / (fHexBlocksPerLine
					* hexDigits / 2);
		} else if (fTextCharsPerLine > 0)
			lineCount = fTargetBlock->Size() / fTextCharsPerLine;

		totalHeight = lineCount * fLineHeight;
		if (totalHeight > 0.0) {
			BRect bounds = Bounds();
			max = totalHeight - bounds.Height();
			scrollBar->SetProportion(bounds.Height() / totalHeight);
			scrollBar->SetSteps(fLineHeight, bounds.Height());
		}
	}
	scrollBar->SetRange(0.0, max);
}

void
MemoryView::_GetNextHexBlock(char* buffer, int32 bufferSize,
	const char* address) const
{
	switch(fHexMode) {
		case HexMode8BitInt:
		{
			snprintf(buffer, bufferSize, "%02" B_PRIx8,
				*((const uint8*)address));
			break;
		}
		case HexMode16BitInt:
		{
			uint16 data = *((const uint16*)address);
			switch(fCurrentEndianMode)
			{
				case EndianModeBigEndian:
				{
					data = B_HOST_TO_BENDIAN_INT16(data);
				}
				break;

				case EndianModeLittleEndian:
				{
					data = B_HOST_TO_LENDIAN_INT16(data);
				}
				break;
			}
			snprintf(buffer, bufferSize, "%04" B_PRIx16,
				data);
			break;
		}
		case HexMode32BitInt:
		{
			uint32 data = *((const uint32*)address);
			switch(fCurrentEndianMode)
			{
				case EndianModeBigEndian:
				{
					data = B_HOST_TO_BENDIAN_INT32(data);
				}
				break;

				case EndianModeLittleEndian:
				{
					data = B_HOST_TO_LENDIAN_INT32(data);
				}
				break;
			}
			snprintf(buffer, bufferSize, "%08" B_PRIx32,
				data);
			break;
		}
		case HexMode64BitInt:
		{
			uint64 data = *((const uint64*)address);
			switch(fCurrentEndianMode)
			{
				case EndianModeBigEndian:
				{
					data = B_HOST_TO_BENDIAN_INT64(data);
				}
				break;

				case EndianModeLittleEndian:
				{
					data = B_HOST_TO_LENDIAN_INT64(data);
				}
				break;
			}
			snprintf(buffer, bufferSize, "%0*" B_PRIx64,
				16, data);
			break;
		}
	}
}


int32
MemoryView::_GetOffsetAt(BPoint point) const
{
	if (fTargetBlock == NULL)
		return -1;

	// TODO: support selection in the text region as well
	if (fHexMode == HexModeNone)
		return -1;

	int32 lineNumber = int32(point.y / fLineHeight);
	int32 charsPerBlock = _GetHexDigitsPerBlock() / 2;
	int32 totalHexBlocks = fTargetBlock->Size() / charsPerBlock;
	int32 lineCount = totalHexBlocks / fHexBlocksPerLine;

	if (lineNumber >= lineCount)
		return -1;

	point.x -= fHexLeft;
	if (point.x < 0)
		point.x = 0;
	else if (point.x > fHexRight)
		point.x = fHexRight;

	float blockWidth = (charsPerBlock * 2 + 1) * fCharWidth;
	int32 containingBlock = int32(floor(point.x / blockWidth));

	return fHexBlocksPerLine * charsPerBlock * lineNumber
		+ containingBlock * charsPerBlock;
}


BPoint
MemoryView::_GetPointForOffset(int32 offset) const
{
	BPoint point;
	if (fHexMode == HexModeNone)
		return point;

	int32 bytesPerLine = fHexBlocksPerLine * _GetHexDigitsPerBlock() / 2;
	int32 line = offset / bytesPerLine;
	int32 lineOffset = offset % bytesPerLine;

	point.x = fHexLeft + (lineOffset * 2 * fCharWidth)
			+ (lineOffset * 2 * fCharWidth / _GetHexDigitsPerBlock());
	point.y = line * fLineHeight;

	return point;
}


void
MemoryView::_RecalcBounds()
{
	fHexLeft = 0;
	fHexRight = 0;
	fTextLeft = 0;
	fTextRight = 0;

	// the left bound is determined by the space taken up by the actual
	// displayed addresses.
	float left = _GetAddressDisplayWidth();
	float width = Bounds().Width() - left;

	if (fHexMode != HexModeNone) {
		int32 hexDigits = _GetHexDigitsPerBlock();
		int32 sizeFactor = 1 + hexDigits;
		if (fTextMode != TextModeNone) {
			float hexProportion = sizeFactor / (float)(sizeFactor
				+ hexDigits / 2);
			float hexWidth = width * hexProportion;
			fTextLeft = left + hexWidth;
			fHexLeft = left;
			// when sharing the display between hex and text,
			// we allocate a 2 character space to separate the views
			hexWidth -= 2 * fCharWidth;
			fHexRight = left + hexWidth;
		} else {
			fHexLeft = left;
			fHexRight = left + width;
		}
	} else if (fTextMode != TextModeNone) {
		fTextLeft = left;
		fTextRight = left + width;
	}
}


float
MemoryView::_GetAddressDisplayWidth() const
{
	return (fTargetAddressSize + 2) * fCharWidth;
}


void
MemoryView::_GetEditCaretRect(BRect& rect) const
{
	if (!fEditMode)
		return;

	int32 byteOffset = fTargetAddress - fTargetBlock->BaseAddress();
	BPoint point = _GetPointForOffset(byteOffset);
	if (fEditLowNybble)
		point.x += fCharWidth;

	rect.left = point.x;
	rect.right = point.x + fCharWidth;
	rect.top = point.y;
	rect.bottom = point.y + fLineHeight;
}


void
MemoryView::_GetSelectionRegion(BRegion& region) const
{
	if (fHexMode == HexModeNone || fTargetBlock == NULL)
		return;

	region.MakeEmpty();
	BPoint startPoint = _GetPointForOffset(fSelectionStart);
	BPoint endPoint = _GetPointForOffset(fSelectionEnd);

	BRect rect;
	if (startPoint.y == endPoint.y) {
		// single line case
		rect.left = startPoint.x;
		rect.top = startPoint.y;
		rect.right = endPoint.x;
		rect.bottom = endPoint.y + fLineHeight;
		region.Include(rect);
	} else {
		float currentLine = startPoint.y;

		// first line
		rect.left = startPoint.x;
		rect.top = startPoint.y;
		rect.right = fHexRight;
		rect.bottom = startPoint.y + fLineHeight;
		region.Include(rect);
		currentLine += fLineHeight;

		// middle region
		if (currentLine < endPoint.y) {
			rect.left = fHexLeft;
			rect.top = currentLine;
			rect.right = fHexRight;
			rect.bottom = endPoint.y;
			region.Include(rect);
		}

		rect.left = fHexLeft;
		rect.top = endPoint.y;
		rect.right = endPoint.x;
		rect.bottom = endPoint.y + fLineHeight;
		region.Include(rect);
	}
}


void
MemoryView::_GetSelectedText(BString& text) const
{
	if (fSelectionStart == fSelectionEnd)
		return;

	text.Truncate(0);
	const uint8* dataSource = fEditMode ? fEditableData : fTargetBlock->Data();

	const char* data = (const char *)dataSource + fSelectionStart;
	int16 blockSize = _GetHexDigitsPerBlock() / 2;
	int32 count = (fSelectionEnd - fSelectionStart)
		/ blockSize;

	char buffer[32];
	for (int32 i = 0; i < count; i++) {
		_GetNextHexBlock(buffer, sizeof(buffer), data);
		data += blockSize;
		text << buffer;
		if (i < count - 1)
			text << " ";
	}
}


void
MemoryView::_CopySelectionToClipboard()
{
	BString text;
	_GetSelectedText(text);

	if (text.Length() > 0) {
		be_clipboard->Lock();
		be_clipboard->Data()->RemoveData("text/plain");
		be_clipboard->Data()->AddData ("text/plain",
			B_MIME_TYPE, text.String(), text.Length());
		be_clipboard->Commit();
		be_clipboard->Unlock();
	}
}


void
MemoryView::_HandleAutoScroll()
{
	BPoint point;
	uint32 buttons;
	GetMouse(&point, &buttons);
	float difference = 0.0;
	int factor = 0;
	BRect visibleRect = Bounds();
	if (point.y < visibleRect.top)
		difference = point.y - visibleRect.top;
	else if (point.y > visibleRect.bottom)
		difference = point.y - visibleRect.bottom;
	if (difference != 0.0) {
		factor = (int)(ceilf(difference / fLineHeight));
		_ScrollByLines(factor);
	}

	MouseMoved(point, B_OUTSIDE_VIEW, NULL);
}


void
MemoryView::_ScrollByLines(int32 lineCount)
{
	BScrollBar* vertical = ScrollBar(B_VERTICAL);
	if (vertical == NULL)
		return;

	float value = vertical->Value();
	vertical->SetValue(value + fLineHeight * lineCount);
}


void
MemoryView::_HandleContextMenu(BPoint point)
{
	int32 offset = _GetOffsetAt(point);
	if (offset < fSelectionStart || offset > fSelectionEnd)
		return;

	BPopUpMenu* menu = new(std::nothrow) BPopUpMenu("Options");
	if (menu == NULL)
		return;

	ObjectDeleter<BPopUpMenu> menuDeleter(menu);
	ObjectDeleter<BMenuItem> itemDeleter;
	ObjectDeleter<BMessage> messageDeleter;
	BMessage* message = NULL;
	BMenuItem* item = NULL;
	if (fSelectionEnd - fSelectionStart == fTargetAddressSize / 2) {
		BMessage* message = new(std::nothrow) BMessage(MSG_INSPECT_ADDRESS);
		if (message == NULL)
			return;

		target_addr_t address;
		if (fTargetAddressSize == 8)
			address = *((uint32*)(fTargetBlock->Data() + fSelectionStart));
		else
			address = *((uint64*)(fTargetBlock->Data() + fSelectionStart));

		if (fCurrentEndianMode == EndianModeBigEndian)
			address = B_HOST_TO_BENDIAN_INT64(address);
		else
			address = B_HOST_TO_LENDIAN_INT64(address);

		messageDeleter.SetTo(message);
		message->AddUInt64("address", address);
		BMenuItem* item = new(std::nothrow) BMenuItem("Inspect", message);
		if (item == NULL)
			return;

		messageDeleter.Detach();
		itemDeleter.SetTo(item);
		if (!menu->AddItem(item))
			return;

		item->SetTarget(Looper());
		itemDeleter.Detach();
	}

	message = new(std::nothrow) BMessage(B_COPY);
	if (message == NULL)
		return;

	messageDeleter.SetTo(message);
	item = new(std::nothrow) BMenuItem("Copy", message);
	if (item == NULL)
		return;

	messageDeleter.Detach();
	itemDeleter.SetTo(item);
	if (!menu->AddItem(item))
		return;

	item->SetTarget(this);
	itemDeleter.Detach();
	menuDeleter.Detach();

	BPoint screenWhere(point);
	ConvertToScreen(&screenWhere);
	BRect mouseRect(screenWhere, screenWhere);
	mouseRect.InsetBy(-4.0, -4.0);
	menu->Go(screenWhere, true, false, mouseRect, true);
}


status_t
MemoryView::_SetupEditableData()
{
	fEditableData = new(std::nothrow) uint8[fTargetBlock->Size()];
	if (fEditableData == NULL)
		return B_NO_MEMORY;

	memcpy(fEditableData, fTargetBlock->Data(), fTargetBlock->Size());

	if (fHexMode != HexMode8BitInt) {
		fHexMode = HexMode8BitInt;
		if (fListener != NULL)
			fListener->HexModeChanged(fHexMode);

		_RecalcScrollBars();
	}

	return B_OK;
}


//#pragma mark - Listener


MemoryView::Listener::~Listener()
{
}
