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

		if (!IsShowingBlock(i))
			continue;

		AddItem(fBlocks.ItemAt(i));
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

