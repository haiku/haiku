/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNICODE_BLOCK_VIEW_H
#define UNICODE_BLOCK_VIEW_H


#include <ListView.h>
#include <ObjectList.h>
#include <String.h>


class BlockListItem : public BStringItem {
public:
							BlockListItem(const char* label, uint32 blockIndex);

			uint32			BlockIndex() const { return fBlockIndex; }

private:
			uint32			fBlockIndex;
};


class UnicodeBlockView : public BListView {
public:
							UnicodeBlockView(const char* name);
	virtual					~UnicodeBlockView();

			void			SetFilter(const char* filter);
			const char*		Filter() const
								{ return fFilter.String(); }

			void			SetCharacterFont(const BFont& font);
			const BFont&	CharacterFont() { return fCharacterFont; }

			void			ShowPrivateBlocks(bool show);
			bool			IsShowingPrivateBlocks() const
								{ return fShowPrivateBlocks; }

			void			ShowContainedBlocksOnly(bool show);
			bool			IsShowingContainedBlocksOnly() const
								{ return fShowContainedBlocksOnly; }

			bool			IsShowingBlock(int32 blockIndex) const;
			void			SelectBlockForCharacter(uint32 character);

private:
			void			_UpdateBlocks();
			void			_CreateBlocks();

private:
			BObjectList<BlockListItem> fBlocks;
			BString			fFilter;
			bool			fShowPrivateBlocks;
			bool			fShowContainedBlocksOnly;
			BFont			fCharacterFont;
			unicode_block	fUnicodeBlocks;
};

#endif	// UNICODE_BLOCK_VIEW_H
