/*
 * Copyright 2004-2018, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DATA_VIEW_H
#define DATA_VIEW_H


#include <Path.h>
#include <String.h>
#include <View.h>


class DataEditor;


enum base_type {
	kHexBase = 16,
	kDecimalBase = 10
};


enum view_focus {
	kNoFocus,
	kHexFocus,
	kAsciiFocus
};


class DataView : public BView {
public:
								DataView(DataEditor& editor);
	virtual						~DataView();

	virtual void				DetachedFromWindow();
	virtual void				AttachedToWindow();
	virtual void				MessageReceived(BMessage* message);
	virtual void				Draw(BRect updateRect);

	virtual void				MouseDown(BPoint where);
	virtual void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* message);
	virtual void				MouseUp(BPoint where);

	virtual void				KeyDown(const char* bytes, int32 numBytes);

	virtual void				WindowActivated(bool active);
	virtual void				MakeFocus(bool focus);
	virtual void				FrameResized(float width, float height);
	virtual void				SetFont(const BFont* font,
									uint32 properties = B_FONT_ALL);
	virtual void				GetPreferredSize(float* _width, float* _height);

			bool				FontSizeFitsBounds() const
									{ return fFitFontSize; }
			float				FontSize() const;
			void				SetFontSize(float point);

			void				UpdateScroller();

			void				MakeVisible(int32 position);
			void				SetSelection(int32 start, int32 end,
									view_focus focus = kNoFocus);
			void				GetSelection(int32& start, int32& end);
			void				InvalidateRange(int32 start, int32 end);

			base_type			Base() const { return fBase; }
			void				SetBase(base_type type);

			const uint8*		DataAt(int32 start);

	static	int32				WidthForFontSize(float size);

private:
			BRect				DataBounds(bool inView = false) const;
			BRect				SelectionFrame(view_focus which, int32 start,
									int32 end);
			int32				PositionAt(view_focus focus, BPoint point,
									view_focus* _newFocus = NULL);

			void				DrawSelectionFrame(view_focus which);
			void				DrawSelectionBlock(view_focus which,
									int32 start, int32 end);
			void				DrawSelectionBlock(view_focus which);
			void				DrawSelection(bool frameOnly = false);
			void				SetActive(bool active);
			void				SetFocus(view_focus which);

			void				UpdateFromEditor(BMessage* message = NULL);
			void				ConvertLine(char* line, off_t offset,
									const uint8* buffer, size_t size);

			bool				AcceptsDrop(const BMessage* message);
			void				InitiateDrag(view_focus focus);
			void				Copy();
			void				Paste();

private:
			DataEditor&			fEditor;
			uint8*				fData;
			size_t				fDataSize;
			off_t				fFileSize;
			size_t				fSizeInView;
			off_t				fOffset;
			float				fAscent;
			int32				fFontHeight;
			float				fCharWidth;
			view_focus			fFocus;
			base_type			fBase;
			bool				fIsActive;
			int32				fStart;
			int32				fEnd;
			int32				fMouseSelectionStart;
			int32				fKeySelectionStart;
			int32				fBitPosition;
			bool				fFitFontSize;
			int32				fDragMessageSize;
			int32				fStoredStart;
			int32				fStoredEnd;
};


static const uint32 kMsgBaseType = 'base';
static const uint32 kMsgUpdateData = 'updt';
static const uint32 kMsgSetSelection = 'ssel';

// observer notices
static const uint32 kDataViewCursorPosition = 'curs';
static const uint32 kDataViewSelection = 'dsel';
static const uint32 kDataViewPreferredSize = 'dvps';

extern bool is_valid_utf8(uint8* data, size_t size);


#endif	/* DATA_VIEW_H */
