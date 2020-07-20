/*
 * Copyright 2007-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TEXTVIEW_H
#define _TEXTVIEW_H


#include <stdint.h>

#include <Locker.h>
#include <View.h>


class BBitmap;
class BClipboard;
class BFile;
class BList;
class BMessageRunner;

struct text_run {
	int32		offset;
	BFont		font;
	rgb_color	color;
};

struct text_run_array {
	int32		count;
	text_run	runs[1];
};

enum undo_state {
	B_UNDO_UNAVAILABLE,
	B_UNDO_TYPING,
	B_UNDO_CUT,
	B_UNDO_PASTE,
	B_UNDO_CLEAR,
	B_UNDO_DROP
};

namespace BPrivate {
	class TextGapBuffer;
}


class BTextView : public BView {
public:
								BTextView(BRect frame, const char* name,
									BRect textRect, uint32 resizeMask,
									uint32 flags
										= B_WILL_DRAW | B_PULSE_NEEDED);
								BTextView(BRect	frame, const char* name,
									BRect textRect, const BFont* initialFont,
									const rgb_color* initialColor,
									uint32 resizeMask, uint32 flags);

								BTextView(const char* name,
									uint32 flags
										= B_WILL_DRAW | B_PULSE_NEEDED);
								BTextView(const char* name,
									const BFont* initialFont,
									const rgb_color* initialColor,
									uint32 flags);

								BTextView(BMessage* archive);

	virtual						~BTextView();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 code,
									const BMessage* dragMessage);
	virtual	void				WindowActivated(bool active);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				Pulse();
	virtual	void				FrameResized(float newWidth, float newHeight);
	virtual	void				MakeFocus(bool focus = true);
	virtual	void				MessageReceived(BMessage* message);

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);
	virtual	status_t			Perform(perform_code code, void* data);

			void				SetText(const char* text,
									const text_run_array* runs = NULL);
			void				SetText(const char* text, int32 length,
									const text_run_array* runs = NULL);
			void				SetText(BFile* file, int32 offset,
									int32 length,
									const text_run_array* runs = NULL);

			void				Insert(const char* text,
									const text_run_array* runs = NULL);
			void				Insert(const char* text, int32 length,
									const text_run_array* runs = NULL);
			void				Insert(int32 offset, const char* text,
									int32 length,
									const text_run_array* runs = NULL);

			void				Delete();
			void				Delete(int32 startOffset, int32 endOffset);

			const char*			Text() const;
			int32				TextLength() const;
			void				GetText(int32 offset, int32 length,
									char* buffer) const;
			uint8				ByteAt(int32 offset) const;

			int32				CountLines() const;
			int32				CurrentLine() const;
			void				GoToLine(int32 lineNumber);

	virtual	void				Cut(BClipboard* clipboard);
	virtual	void				Copy(BClipboard* clipboard);
	virtual	void				Paste(BClipboard* clipboard);
			void				Clear();

	virtual	bool				AcceptsPaste(BClipboard* clipboard);
	virtual	bool				AcceptsDrop(const BMessage* message);

	virtual	void				Select(int32 startOffset, int32 endOffset);
			void				SelectAll();
			void				GetSelection(int32* _start, int32* _end) const;

			void				SetFontAndColor(const BFont* font,
									uint32 mode = B_FONT_ALL,
									const rgb_color* color = NULL);
			void				SetFontAndColor(int32 startOffset,
									int32 endOffset, const BFont* font,
									uint32 mode = B_FONT_ALL,
									const rgb_color* color = NULL);

			void				GetFontAndColor(int32 offset, BFont* _font,
									rgb_color* _color = NULL) const;
			void				GetFontAndColor(BFont* _font, uint32* _mode,
									rgb_color* _color = NULL,
									bool* _sameColor = NULL) const;

			void				SetRunArray(int32 startOffset, int32 endOffset,
									const text_run_array* runs);
			text_run_array*		RunArray(int32 startOffset, int32 endOffset,
									int32* _size = NULL) const;

			int32				LineAt(int32 offset) const;
			int32				LineAt(BPoint point) const;
			BPoint				PointAt(int32 offset,
									float* _height = NULL) const;
			int32				OffsetAt(BPoint point) const;
			int32				OffsetAt(int32 line) const;

	virtual	void				FindWord(int32 offset, int32* _fromOffset,
									int32* _toOffset);

	virtual	bool				CanEndLine(int32 offset);

			float				LineWidth(int32 lineNumber = 0) const;
			float				LineHeight(int32 lineNumber = 0) const;
			float				TextHeight(int32 startLine,
									int32 endLine) const;

			void				GetTextRegion(int32 startOffset,
									int32 endOffset, BRegion* outRegion) const;

	virtual	void				ScrollToOffset(int32 offset);
			void				ScrollToSelection();

			void				Highlight(int32 startOffset, int32 endOffset);

			void				SetTextRect(BRect rect);
			BRect				TextRect() const;
			void				SetInsets(float left, float top, float right,
									float bottom);
			void				GetInsets(float* _left, float* _top,
									float* _right, float* _bottom) const;

			void				SetStylable(bool stylable);
			bool				IsStylable() const;
			void				SetTabWidth(float width);
			float				TabWidth() const;
			void				MakeSelectable(bool selectable = true);
			bool				IsSelectable() const;
			void				MakeEditable(bool editable = true);
			bool				IsEditable() const;
			void				SetWordWrap(bool wrap);
			bool				DoesWordWrap() const;
			void				SetMaxBytes(int32 max);
			int32				MaxBytes() const;
			void				DisallowChar(uint32 character);
			void				AllowChar(uint32 character);
			void				SetAlignment(alignment align);
			alignment			Alignment() const;
			void				SetAutoindent(bool state);
			bool				DoesAutoindent() const;
			void				SetColorSpace(color_space colors);
			color_space			ColorSpace() const;
			void				MakeResizable(bool resize,
									BView* resizeView = NULL);
			bool				IsResizable() const;
			void				SetDoesUndo(bool undo);
			bool				DoesUndo() const;
			void				HideTyping(bool enabled);
			bool				IsTypingHidden() const;

	virtual	void				ResizeToPreferred();
	virtual	void				GetPreferredSize(float* _width, float* _height);

	virtual	void				AllAttached();
	virtual	void				AllDetached();

	static	text_run_array*		AllocRunArray(int32 entryCount,
									int32* outSize = NULL);
	static	text_run_array*		CopyRunArray(const text_run_array* orig,
									int32 countDelta = 0);
	static	void				FreeRunArray(text_run_array* array);
	static	void*				FlattenRunArray(const text_run_array* runArray,
									int32* _size = NULL);
	static	text_run_array*		UnflattenRunArray(const void* data,
									int32* _size = NULL);

protected:
	virtual	void				InsertText(const char* text, int32 length,
									int32 offset, const text_run_array* runs);
	virtual	void				DeleteText(int32 fromOffset, int32 toOffset);

public:
	virtual	void				Undo(BClipboard* clipboard);
			undo_state			UndoState(bool* isRedo) const;

protected:
	virtual	void				GetDragParameters(BMessage* drag,
									BBitmap** _bitmap, BPoint* point,
									BHandler** _handler);

	virtual	void				LayoutInvalidated(bool descendants);
	virtual	void				DoLayout();

public:
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual	bool				HasHeightForWidth();
	virtual	void				GetHeightForWidth(float width, float* min,
									float* max, float* preferred);

private:
	// FBC padding and forbidden methods
	virtual	void				_ReservedTextView3();
	virtual	void				_ReservedTextView4();
	virtual	void				_ReservedTextView5();
	virtual	void				_ReservedTextView6();
	virtual	void				_ReservedTextView7();
	virtual	void				_ReservedTextView8();
	virtual	void				_ReservedTextView9();
	virtual	void				_ReservedTextView10();
	virtual	void				_ReservedTextView11();
	virtual	void				_ReservedTextView12();

private:
			class InlineInput;
			struct LayoutData;
			class LineBuffer;
			class StyleBuffer;
			class TextTrackState;
			class UndoBuffer;

			// UndoBuffer derivatives
			class CutUndoBuffer;
			class PasteUndoBuffer;
			class ClearUndoBuffer;
			class DropUndoBuffer;
			class TypingUndoBuffer;

			friend class TextTrackState;

			void				_InitObject(BRect textRect,
									const BFont* initialFont,
									const rgb_color* initialColor);

			void				_ValidateLayoutData();
			void				_ResetTextRect();

			void				_HandleBackspace(int32 modifiers = -1);
			void				_HandleArrowKey(uint32 arrowKey,
									int32 modifiers = -1);
			void				_HandleDelete(int32 modifiers = -1);
			void				_HandlePageKey(uint32 pageKey,
									int32 modifiers = -1);
			void				_HandleAlphaKey(const char* bytes,
									int32 numBytes);

			void				_Refresh(int32 fromOffset, int32 toOffset,
									int32 scrollTo = INT32_MIN);
			void				_RecalculateLineBreaks(int32* startLine,
									int32* endLine);
			int32				_FindLineBreak(int32 fromOffset,
									float* _ascent, float* _descent,
									float* inOutWidth);

			float				_StyledWidth(int32 fromOffset, int32 length,
									float* _ascent = NULL,
									float* _descent = NULL) const;
			float				_TabExpandedStyledWidth(int32 offset,
									int32 length, float* _ascent = NULL,
									float* _descent = NULL) const;

			float				_ActualTabWidth(float location) const;

			void				_DoInsertText(const char* text, int32 length,
									int32 offset, const text_run_array* runs);

			void				_DoDeleteText(int32 fromOffset,
									int32 toOffset);

			void				_DrawLine(BView* view, const int32 &startLine,
									const int32& startOffset,
									const bool& erase, BRect& eraseRect,
									BRegion& inputRegion);

			void				_DrawLines(int32 startLine, int32 endLine,
									int32 startOffset = -1,
									bool erase = false);
			void				_RequestDrawLines(int32 startLine,
									int32 endLine);

			void				_DrawCaret(int32 offset, bool visible);
			void				_ShowCaret();
			void				_HideCaret();
			void				_InvertCaret();
			void				_DragCaret(int32 offset);

			void				_StopMouseTracking();
			bool				_PerformMouseUp(BPoint where);
			bool				_PerformMouseMoved(BPoint where, uint32 code);

			void				_TrackMouse(BPoint where,
									const BMessage* message,
									bool force = false);

			void				_TrackDrag(BPoint where);
			void				_InitiateDrag();
			bool				_MessageDropped(BMessage* message,
									BPoint where, BPoint offset);

			void				_PerformAutoScrolling();
			void				_UpdateScrollbars();
			void				_ScrollBy(float horizontalStep,
									float verticalStep);
			void				_ScrollTo(float x, float y);

			void				_AutoResize(bool doRedraw = true);

			void				_NewOffscreen(float padding = 0.0);
			void				_DeleteOffscreen();

			void				_Activate();
			void				_Deactivate();

			void				_NormalizeFont(BFont* font);

			void				_SetRunArray(int32 startOffset, int32 endOffset,
									const text_run_array* runs);

			void				_ApplyStyleRange(int32 fromOffset,
									int32 toOffset,
									uint32 mode = B_FONT_ALL,
									const BFont* font = NULL,
									const rgb_color* color = NULL,
									bool syncNullStyle = true);

			uint32				_CharClassification(int32 offset) const;
			int32				_NextInitialByte(int32 offset) const;
			int32				_PreviousInitialByte(int32 offset) const;

			int32				_PreviousLineStart(int32 offset);
			int32				_NextLineEnd(int32 offset);

			int32				_PreviousWordBoundary(int32 offset);
			int32				_NextWordBoundary(int32 offset);

			int32				_PreviousWordStart(int32 offset);
			int32				_NextWordEnd(int32 offset);

			bool				_GetProperty(BMessage* specifier, int32 form,
									const char* property, BMessage* reply);
			bool				_SetProperty(BMessage* specifier, int32 form,
									const char* property, BMessage* reply);
			bool				_CountProperties(BMessage* specifier,
									int32 form, const char* property,
									BMessage* reply);

			void				_HandleInputMethodChanged(BMessage* message);
			void				_HandleInputMethodLocationRequest();
			void				_CancelInputMethod();

			int32				_LineAt(int32 offset) const;
			int32				_LineAt(const BPoint& point) const;
			bool				_IsOnEmptyLastLine(int32 offset) const;

			float				_NullStyleHeight() const;

			void				_ShowContextMenu(BPoint where);

			void				_FilterDisallowedChars(char* text,
									ssize_t& length, text_run_array* runArray);

			void				_UpdateInsets(const BRect& rect);

private:
			BPrivate::TextGapBuffer*	fText;
			LineBuffer*			fLines;
			StyleBuffer*		fStyles;
			BRect				fTextRect;
			int32				fSelStart;
			int32				fSelEnd;
			bool				fCaretVisible;
			bigtime_t			fCaretTime;
			int32				fCaretOffset;
			int32				fClickCount;
			bigtime_t			fClickTime;
			int32				fDragOffset;
			uint8				fCursor;
			bool				fActive;
			bool				fStylable;
			float				fTabWidth;
			bool				fSelectable;
			bool				fEditable;
			bool				fWrap;
			int32				fMaxBytes;
			BList*				fDisallowedChars;
			alignment			fAlignment;
			bool				fAutoindent;
			BBitmap* 			fOffscreen;
			color_space			fColorSpace;
			bool				fResizable;
			BView*				fContainerView;
			UndoBuffer*			fUndo;
			InlineInput*		fInline;
			BMessageRunner*		fDragRunner;
			BMessageRunner*		fClickRunner;
			BPoint				fWhere;
			TextTrackState*		fTrackingMouse;

			float				fMinTextRectWidth;
			LayoutData*			fLayoutData;
			int32				fLastClickOffset;

			bool				fInstalledNavigateCommandWordwiseShortcuts : 1;
			bool				fInstalledNavigateOptionWordwiseShortcuts : 1;
			bool				fInstalledNavigateOptionLinewiseShortcuts : 1;
			bool				fInstalledNavigateHomeEndDocwiseShortcuts : 1;

			bool				fInstalledSelectCommandWordwiseShortcuts : 1;
			bool				fInstalledSelectOptionWordwiseShortcuts : 1;
			bool				fInstalledSelectOptionLinewiseShortcuts : 1;
			bool				fInstalledSelectHomeEndDocwiseShortcuts : 1;

			bool				fInstalledRemoveCommandWordwiseShortcuts : 1;
			bool				fInstalledRemoveOptionWordwiseShortcuts : 1;

			uint32				_reserved[6];
};

#endif	// _TEXTVIEW_H
