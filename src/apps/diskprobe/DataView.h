/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef DATA_VIEW_H
#define DATA_VIEW_H


#include <View.h>
#include <String.h>
#include <Path.h>


class DataEditor;

static const uint32 kMsgBaseType = 'base';

enum base_type {
	kHexBase,
	kDecimalBase
};

enum view_focus {
	kNoFocus,
	kHexFocus,
	kAsciiFocus
};

class DataView : public BView {
	public:
		DataView(BRect rect, DataEditor &editor);
		virtual ~DataView();

		virtual void DetachedFromWindow();
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		virtual void Draw(BRect updateRect);

		virtual void MouseDown(BPoint where);
		virtual void MouseMoved(BPoint where, uint32 transit, const BMessage *message);
		virtual void MouseUp(BPoint where);

		virtual void WindowActivated(bool active);
		virtual void FrameResized(float width, float height);
		virtual void SetFont(const BFont *font, uint32 properties = B_FONT_ALL);
		virtual void GetPreferredSize(float *_width, float *_height);

		void SetFontSize(float point);
		void UpdateScroller();

		void SetSelection(int32 start, int32 end, view_focus focus = kNoFocus);
		void GetSelection(int32 &start, int32 &end);
		void InvalidateRange(int32 start, int32 end);

		base_type Base() const { return fBase; }
		void SetBase(base_type type);

	private:
		BRect SelectionFrame(view_focus which, int32 start, int32 end);
		int32 PositionAt(view_focus focus, BPoint point, view_focus *_newFocus = NULL);

		void DrawSelectionFrame(view_focus which);
		void DrawSelectionBlock(view_focus which);
		void DrawSelection();
		void SetFocus(view_focus which);

		void UpdateFromEditor(BMessage *message = NULL);
		void ConvertLine(char *line, off_t offset, const uint8 *buffer, size_t size);

		DataEditor	&fEditor;
		int32		fPositionLength;
		uint8		*fData;
		size_t		fDataSize;
		off_t		fOffset;
		float		fAscent;
		int32		fFontHeight;
		float		fCharWidth;
		view_focus	fFocus;
		base_type	fBase;
		bool		fIsActive;
		int32		fStart, fEnd;
		int32		fMouseSelectionStart;
};

#endif	/* DATA_VIEW_H */
