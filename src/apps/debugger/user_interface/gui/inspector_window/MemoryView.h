/*
 * Copyright 2011-2015, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef MEMORY_VIEW_H
#define MEMORY_VIEW_H


#include <set>

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
};

enum {
	EndianModeLittleEndian = 0,
	EndianModeBigEndian = 1
};

enum {
	TextModeNone = 0,
	TextModeASCII
};


class BMessageRunner;

class Team;
class TeamMemoryBlock;


class MemoryView : public BView {
public:
	class Listener;

								MemoryView(::Team* team, Listener* listener);
	virtual						~MemoryView();

	static MemoryView*			Create(::Team* team, Listener* listener);
									// throws

			void				SetTargetAddress(TeamMemoryBlock* block,
									target_addr_t address);
			void				UnsetListener();

	inline	bool				GetEditMode() const
									{ return fEditMode; }
			status_t			SetEditMode(bool enabled);

	inline	void*				GetEditedData() const
									{ return fEditableData; }

			void				CommitChanges();
			void				RevertChanges();

	virtual	void				AttachedToWindow();
	virtual void				Draw(BRect rect);
	virtual void				FrameResized(float width, float height);
	virtual void				KeyDown(const char* bytes, int32 numBytes);
	virtual void				MakeFocus(bool isFocused);
	virtual void				MessageReceived(BMessage* message);
	virtual void				MouseDown(BPoint point);
	virtual void				MouseMoved(BPoint point, uint32 transit,
									const BMessage* dragMessage);
	virtual void				MouseUp(BPoint point);
			void				ScrollToSelection();
	virtual void				TargetedByScrollView(BScrollView* scrollView);


	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();
	virtual	BSize				MaxSize();

private:
	void						_Init();
	void						_RecalcScrollBars();
	void						_GetNextHexBlock(char* buffer,
									int32 bufferSize,
									const char* address) const;

	int32						_GetOffsetAt(BPoint point) const;
	BPoint						_GetPointForOffset(int32 offset) const;
	void						_RecalcBounds();
	float						_GetAddressDisplayWidth() const;

	inline int32				_GetHexDigitsPerBlock() const
									{ return 1 << fHexMode; };

	void						_GetEditCaretRect(BRect& rect) const;
	void						_GetSelectionRegion(BRegion& region) const;
	void						_GetSelectedText(BString& text) const;
	void						_CopySelectionToClipboard();

	void						_HandleAutoScroll();
	void						_ScrollByLines(int32 lineCount);
	void						_HandleContextMenu(BPoint point);

	status_t					_SetupEditableData();

private:
	typedef std::set<int32>		ModifiedIndexSet;

private:
	::Team*						fTeam;
	TeamMemoryBlock*			fTargetBlock;
	uint8*						fEditableData;
	ModifiedIndexSet			fEditedOffsets;
	target_addr_t				fTargetAddress;
	bool						fEditMode;
	bool						fEditLowNybble;
	float						fCharWidth;
	float						fLineHeight;
	int32						fTargetAddressSize;
	int32						fTextCharsPerLine;
	int32						fHexBlocksPerLine;
	int32						fCurrentEndianMode;
	int32						fHexMode;
	int32						fTextMode;
	float						fHexLeft;
	float						fHexRight;
	float						fTextLeft;
	float						fTextRight;
	int32						fSelectionBase;
	int32						fSelectionStart;
	int32						fSelectionEnd;
	BMessageRunner*				fScrollRunner;

	bool						fTrackingMouse;

	Listener*					fListener;
};


class MemoryView::Listener {
public:
	virtual						~Listener();

	virtual	void				TargetAddressChanged(target_addr_t address)
									= 0;

	virtual	void				HexModeChanged(int32 newMode) = 0;
	virtual	void				TextModeChanged(int32 newMode) = 0;
	virtual	void				EndianModeChanged(int32 newMode) = 0;
};


#endif // MEMORY_VIEW_H
