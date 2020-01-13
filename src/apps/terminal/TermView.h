/*
 * Copyright 2001-2013, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai.
 *
 * Distributed under the terms of the MIT license.
 * Authors:
 *		Stefano Ceccherini, stefano.ceccherini@gmail.com
 *		Kian Duffy, myob@users.sourceforge.net
 *		Simon South, simon@simonsouth.net
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Siarzhuk Zharski, zharik@gmx.li
 */
#ifndef TERMVIEW_H
#define TERMVIEW_H


#include <Autolock.h>
#include <HashMap.h>
#include <InterfaceDefs.h>
#include <Messenger.h>
#include <ObjectList.h>
#include <String.h>
#include <View.h>

#include "TermPos.h"
#include "TermViewHighlight.h"


class ActiveProcessInfo;
class BClipboard;
class BMessageRunner;
class BScrollBar;
class BScrollView;
class BString;
class BStringView;
class BasicTerminalBuffer;
class DefaultCharClassifier;
class InlineInput;
class ResizeWindow;
class ShellInfo;
class ShellParameters;
class TermBuffer;
class TerminalBuffer;
class Shell;


class TermView : public BView, private TermViewHighlighter {
public:
			class Listener;

			typedef TermViewHighlighter Highlighter;
			typedef TermViewHighlight Highlight;

public:
								TermView(BRect frame,
									const ShellParameters& shellParameters,
									int32 historySize);
								TermView(int rows, int columns,
									const ShellParameters& shellParameters,
									int32 historySize);
								TermView(BMessage* archive);
								~TermView();

	static	BArchivable*		Instantiate(BMessage* data);
	virtual status_t			Archive(BMessage* data, bool deep = true) const;

	virtual void				GetPreferredSize(float* _width, float* _height);

			bool				IsShellBusy() const;
			bool				GetActiveProcessInfo(
									ActiveProcessInfo& _info) const;
			bool				GetShellInfo(ShellInfo& _info) const;

			const char*			TerminalName() const;

	inline	TerminalBuffer*		TextBuffer() const	{ return fTextBuffer; }

			void				GetTermFont(BFont* font) const;
			void				SetTermFont(const BFont* font);

			void				GetFontSize(float* width, float* height);
			int					Rows() const;
			int					Columns() const;
			BRect				SetTermSize(int rows, int columns,
									bool notifyShell);
			void				SetTermSize(BRect rect,
									bool notifyShell = false);
			void				GetTermSizeFromRect(const BRect &rect,
									int *rows, int *columns);

			void				SetTextColor(rgb_color fore, rgb_color back);
			void				SetCursorColor(rgb_color fore, rgb_color back);
			void				SetSelectColor(rgb_color fore, rgb_color back);
			void				SetTermColor(uint index, rgb_color color,
									bool dynamic = false);

			int					Encoding() const;
			void				SetEncoding(int encoding);

			void				SetScrollBar(BScrollBar* scrollBar);
			BScrollBar*			ScrollBar() const { return fScrollBar; };

			void				SetKeymap(const key_map* keymap,
									const char* chars);
			void				SetUseOptionAsMetaKey(bool enable);

			void				SetMouseClipboard(BClipboard *);

			void				MakeDebugSnapshots();
			void				StartStopDebugCapture();

			void				SwitchCursorBlinking(bool blinkingOn);

			// edit functions
			void				Copy(BClipboard* clipboard);
			void				Paste(BClipboard* clipboard);
			void				SelectAll();
			void				Clear();

			// Other
			void				GetFrameSize(float* width, float* height);
			bool				Find(const BString& str, bool forwardSearch,
									bool matchCase, bool matchWord);
			void				GetSelection(BString& string);

			bool				CheckShellGone() const;

			void				InitiateDrag();

			void				DisableResizeView(int32 disableCount = 1);

			void				SetListener(Listener* listener)
									{ fListener = listener; }

protected:
	virtual void				AttachedToWindow();
	virtual void				DetachedFromWindow();
	virtual void				Draw(BRect updateRect);
	virtual void				WindowActivated(bool active);
	virtual void				MakeFocus(bool focusState = true);
	virtual void				KeyDown(const char* bytes, int32 numBytes);

	virtual void				MouseDown(BPoint where);
	virtual void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* message);
	virtual void				MouseUp(BPoint where);

	virtual void				FrameResized(float width, float height);
	virtual void				MessageReceived(BMessage* message);

	virtual void				ScrollTo(BPoint where);
	virtual void				TargetedByScrollView(BScrollView *scrollView);

	virtual status_t			GetSupportedSuites(BMessage* message);
	virtual BHandler*			ResolveSpecifier(BMessage* message, int32 index,
									BMessage* specifier, int32 form,
									const char* property);

private:
			class TextBufferSyncLocker;
			friend class TextBufferSyncLocker;

			class State;
			class StandardBaseState;
			class DefaultState;
			class SelectState;
			class HyperLinkState;
			class HyperLinkMenuState;

			friend class State;
			friend class StandardBaseState;
			friend class DefaultState;
			friend class SelectState;
			friend class HyperLinkState;
			friend class HyperLinkMenuState;

			typedef BObjectList<Highlight> HighlightList;

private:
			// TermViewHighlighter
	virtual	rgb_color			ForegroundColor();
	virtual	rgb_color			BackgroundColor();

private:
			// point and text offset conversion
	inline	int32				_LineAt(float y);
	inline	float				_LineOffset(int32 index);
			TermPos				_ConvertToTerminal(const BPoint& point);
	inline	BPoint				_ConvertFromTerminal(const TermPos& pos);

	inline	void				_InvalidateTextRect(int32 x1, int32 y1,
									int32 x2, int32 y2);

			status_t			_InitObject(
									const ShellParameters& shellParameters);

			status_t			_AttachShell(Shell* shell);
			void				_DetachShell();

			void				_Activate();
			void				_Deactivate();

			void				_DrawLinePart(float x1, float y1, uint32 attr,
									char* buffer, int32 width,
									Highlight* highlight, bool cursor,
									BView* inView);
			void				_DrawCursor();
			void				_InvalidateTextRange(TermPos start,
									TermPos end);

			bool				_IsCursorVisible() const;
			void				_BlinkCursor();
			void				_ActivateCursor(bool invalidate);

			void				_DoPrint(BRect updateRect);
			void				_UpdateScrollBarRange();
			void				_SecondaryMouseButtonDropped(BMessage* message);
			void				_DoSecondaryMouseDropAction(BMessage* message);
			void				_DoFileDrop(entry_ref &ref);

			void				_SynchronizeWithTextBuffer(
									int32 visibleDirtyTop,
									int32 visibleDirtyBottom);
			void				_VisibleTextBufferChanged();

			void				_WritePTY(const char* text, int32 numBytes);

			// selection
			float				_MouseDistanceSinceLastClick(BPoint where);
			void				_Select(TermPos start, TermPos end,
									bool inclusive, bool setInitialSelection);
			void				_ExtendSelection(TermPos, bool inclusive,
									bool useInitialSelection);
			void				_Deselect();
			bool				_HasSelection() const;
			void				_SelectWord(BPoint where, bool extend,
									bool useInitialSelection);
			void				_SelectLine(BPoint where, bool extend,
									bool useInitialSelection);

			void				_AddHighlight(Highlight* highlight);
			void				_RemoveHighlight(Highlight* highlight);
			bool				_ClearHighlight(Highlight* highlight);

			Highlight*			_CheckHighlightRegion(const TermPos& pos) const;
			Highlight*			_CheckHighlightRegion(int32 row,
									int32 firstColumn, int32& lastColumn) const;

			void				_UpdateSIGWINCH();

			void				_ScrollTo(float y, bool scrollGfx);
			void				_ScrollToRange(TermPos start, TermPos end);

			void				_SendMouseEvent(int32 button, int32 mode,
									int32 x, int32 y, bool motion);

			void				_DrawInlineMethodString();
			void				_HandleInputMethodChanged(BMessage* message);
			void				_HandleInputMethodLocationRequest();
			void				_CancelInputMethod();

			void				_UpdateModifiers();

			void				_NextState(State* state);

private:
			Listener*			fListener;
			Shell*				fShell;

			BMessageRunner*		fWinchRunner;
			BMessageRunner*		fCursorBlinkRunner;
			BMessageRunner*		fAutoScrollRunner;
			BMessageRunner*		fResizeRunner;
			BStringView*		fResizeView;
			DefaultCharClassifier* fCharClassifier;

			// Font and Width
			BFont				fHalfFont;
			BFont				fBoldFont;
			float					fFontWidth;
			int					fFontHeight;
			int					fFontAscent;
			struct escapement_delta fEscapement;
			bool				fEmulateBold;
			bool				fAllowBold;

			// frame resized flag.
			bool				fFrameResized;
			int32				fResizeViewDisableCount;

			// Cursor Blinking, draw flag.
			bigtime_t			fLastActivityTime;
			int32				fCursorState;
			int					fCursorStyle;
			bool				fCursorBlinking;
			bool				fCursorHidden;

			// Cursor position.
			TermPos				fCursor;

			// Terminal rows and columns.
			int					fColumns;
			int					fRows;

			int					fEncoding;
			bool				fActive;

			// Object pointer.
			TerminalBuffer*		fTextBuffer;
			BasicTerminalBuffer* fVisibleTextBuffer;
			bool				fVisibleTextBufferChanged;
			BScrollBar*			fScrollBar;
			InlineInput*		fInline;

			// Color and Attribute.
			rgb_color			fTextForeColor;
			rgb_color			fTextBackColor;
			rgb_color			fCursorForeColor;
			rgb_color			fCursorBackColor;
			rgb_color			fSelectForeColor;
			rgb_color			fSelectBackColor;

			// Scroll Region
			float				fScrollOffset;
			int32				fScrBufSize;
				// TODO: That's the history capacity -- only needed
				// until the text buffer is created.
			float				fAutoScrollSpeed;

			// redraw management
			bigtime_t			fLastSyncTime;
			int32				fScrolledSinceLastSync;
			BMessageRunner*		fSyncRunner;
			bool				fConsiderClockedSync;

			// selection
			Highlight			fSelection;
			TermPos				fInitialSelectionStart;
			TermPos				fInitialSelectionEnd;
			BPoint				fLastClickPoint;

			HighlightList		fHighlights;

			// keyboard
			const key_map*		fKeymap;
			const char*			fKeymapChars;
			HashMap<HashKey32<int32>, const int32(*)[128]>
								fKeymapTableForModifiers;
			bool				fUseOptionAsMetaKey;
			bool				fInterpretMetaKey;
			bool				fMetaKeySendsEscape;

			// mouse
			int32				fMouseButtons;
			int32				fModifiers;
			TermPos				fPrevPos;
			bool				fReportX10MouseEvent;
			bool				fReportNormalMouseEvent;
			bool				fReportButtonMouseEvent;
			bool				fReportAnyMouseEvent;
			bool				fEnableExtendedMouseCoordinates;
			BClipboard*			fMouseClipboard;

			// states
			DefaultState*		fDefaultState;
			SelectState*		fSelectState;
			HyperLinkState*		fHyperLinkState;
			HyperLinkMenuState*	fHyperLinkMenuState;
			State*				fActiveState;
};


class TermView::Listener {
public:
	virtual						~Listener();

	// all hooks called in the window thread
	virtual	void				NotifyTermViewQuit(TermView* view,
									int32 reason);
	virtual	void				SetTermViewTitle(TermView* view,
									const char* title);
	virtual	void				PreviousTermView(TermView* view);
	virtual	void				NextTermView(TermView* view);
};


#endif // TERMVIEW_H
