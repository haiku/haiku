/*
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef MEMORY_VIEW_H
#define MEMORY_VIEW_H


#include <View.h>

#include "Types.h"


enum {
	MSG_SET_HEX_MODE 	= 'sehe',
	MSG_SET_ENDIAN_MODE	= 'seme',
	MSG_SET_TEXT_MODE	= 'stme'
};

enum {
	HexModeNone	= 0,
	HexMode8BitInt,
	HexMode16BitInt,
	HexMode32BitInt,
	HexMode64BitInt
	// TODO: floating point representation?
};

enum {
	EndianModeLittleEndian = 0,
	EndianModeBigEndian = 1
};

enum {
	TextModeNone = 0,
	TextModeASCII
};


class Team;


class TeamMemoryBlock;


class MemoryView : public BView {
public:
								MemoryView(::Team* team);
	virtual						~MemoryView();

	static MemoryView*			Create(::Team* team);
									// thrws

			void				SetTargetAddress(TeamMemoryBlock* block,
									target_addr_t address);

	virtual	void				AttachedToWindow();
	virtual void				Draw(BRect rect);
	virtual void				FrameResized(float width, float height);
	virtual void				KeyDown(const char* bytes, int32 numBytes);
	virtual void				MakeFocus(bool isFocused);
	virtual void				MessageReceived(BMessage* message);
	virtual void				MouseDown(BPoint point);
			void				ScrollToSelection();
	virtual void				TargetedByScrollView(BScrollView* scrollView);

private:
	void						_Init();
	void						_RecalcScrollBars();
	void						_GetNextHexBlock(char* buffer,
									int32 bufferSize, const char* address);

private:
	::Team*						fTeam;
	TeamMemoryBlock*			fTargetBlock;
	target_addr_t				fTargetAddress;
	float						fCharWidth;
	float						fLineHeight;
	int32						fTextCharsPerLine;
	int32						fHexBlocksPerLine;
	int32						fCurrentEndianMode;
	int32						fHexMode;
	int32						fTextMode;
};

#endif // MEMORY_VIEW_H
