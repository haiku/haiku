/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "UnicodeBlockView.h"

#include <stdio.h>
#include <string.h>

#include "UnicodeBlocks.h"


BlockListItem::BlockListItem(const char* label, uint32 blockIndex)
	: BStringItem(label),
	fBlockIndex(blockIndex)
{
}


//	#pragma mark -


UnicodeBlockView::UnicodeBlockView(const char* name)
	: BListView(name),
	fBlocks(kNumUnicodeBlocks, true),
	fShowPrivateBlocks(false),
	fShowContainedBlocksOnly(false)
{
	_CreateBlocks();
}


UnicodeBlockView::~UnicodeBlockView()
{
}


void
UnicodeBlockView::SetFilter(const char* filter)
{
	fFilter = filter;
	_UpdateBlocks();
}

void
UnicodeBlockView::SetCharacterFont(const BFont& font)
{
	fCharacterFont = font;
	fUnicodeBlocks = fCharacterFont.Blocks();
	_UpdateBlocks();
}


void
UnicodeBlockView::ShowPrivateBlocks(bool show)
{
	if (fShowPrivateBlocks == show)
		return;

	fShowPrivateBlocks = show;
	_UpdateBlocks();
}


void
UnicodeBlockView::ShowContainedBlocksOnly(bool show)
{
	if (fShowContainedBlocksOnly == show)
		return;

	fShowContainedBlocksOnly = show;
	_UpdateBlocks();
}


bool
UnicodeBlockView::IsShowingBlock(int32 blockIndex) const
{
	if (blockIndex < 0 || blockIndex >= (int32)kNumUnicodeBlocks)
		return false;

	if (!fShowPrivateBlocks && kUnicodeBlocks[blockIndex].private_block)
		return false;

	// the reason for two checks is BeOS compatibility.
	// The Includes method checks for unicode blocks as
	// defined by Be, but there are only 71 such blocks.
	// The rest of the blocks (denoted by kNoBlock) need to
	// be queried by searching for the start and end codepoints
	// via the IncludesBlock method.
	if (fShowContainedBlocksOnly) {
		if (kUnicodeBlocks[blockIndex].block != kNoBlock
			&& !fUnicodeBlocks.Includes(
				kUnicodeBlocks[blockIndex].block))
			return false;

		if (!fCharacterFont.IncludesBlock(
				kUnicodeBlocks[blockIndex].start,
				kUnicodeBlocks[blockIndex].end))
			return false;
	}

	return true;
}


void
UnicodeBlockView::_UpdateBlocks()
{
	MakeEmpty();

	for (int32 i = 0; i < fBlocks.CountItems(); i++) {
		if (fFilter.Length() != 0) {
			if (strcasestr(kUnicodeBlocks[i].name, fFilter.String()) == NULL)
				continue;
		}

		AddItem(fBlocks.ItemAt(i));

		fBlocks.ItemAt(i)->SetEnabled(IsShowingBlock(i));
	}
}


void
UnicodeBlockView::_CreateBlocks()
{
	float minWidth = 0;
	for (uint32 i = 0; i < kNumUnicodeBlocks; i++) {
		BlockListItem* item = new BlockListItem(kUnicodeBlocks[i].name, i);
		fBlocks.AddItem(item);

		float width = StringWidth(item->Text());
		if (minWidth < width)
			minWidth = width;
	}

	SetExplicitMinSize(BSize(minWidth / 2, 32));
	SetExplicitMaxSize(BSize(minWidth, B_SIZE_UNSET));

	_UpdateBlocks();
}


void
UnicodeBlockView::SelectBlockForCharacter(uint32 character)
{
	// find block containing the character
	int32 blockNumber = BlockForCharacter(character);

	if (blockNumber > 0) {
		BlockListItem* block = fBlocks.ItemAt(blockNumber);

		int32 blockIndex = IndexOf(block);

		if (blockIndex >= 0) {
			Select(blockIndex);
			ScrollToSelection();
		}
	}
}
