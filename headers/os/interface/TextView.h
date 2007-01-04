/*******************************************************************************
/
/	File:			TextView.h
/
/   Description:    BTextView displays and manages styled text.
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _TEXTVIEW_H
#define _TEXTVIEW_H

#include <BeBuild.h>
#include <View.h>

/*----------------------------------------------------------------*/
/*----- BTextView structures and definitions ---------------------*/

struct text_run {
	int32			offset;		/* byte offset of first character of run*/
	BFont			font;		/* font of run*/
	rgb_color		color;		/* color of run*/
};

struct text_run_array {
	int32			count;		/* number of text runs*/
	text_run		runs[1];	/* array of count number of runs*/
};

enum undo_state {
	B_UNDO_UNAVAILABLE,
	B_UNDO_TYPING,
	B_UNDO_CUT,
	B_UNDO_PASTE,
	B_UNDO_CLEAR,
	B_UNDO_DROP
};


#if _PR2_COMPATIBLE_
extern "C" void _ReservedTextView2__9BTextViewFv(BTextView	*object, 
												 BMessage	*drag, 
												 BBitmap	**bitmap, 
												 BPoint		*point, 
												 BHandler	**handler);
#endif


class BBitmap;
class BClipboard;
class BFile;
class BList;
class _BTextGapBuffer_;
class _BLineBuffer_;
class _BStyleBuffer_;
class _BWidthBuffer_;
class _BUndoBuffer_;
class _BInlineInput_;
class _BTextTrackState_;
class _BTextChangeResult_;

extern "C" status_t	_init_interface_kit_();

/*----------------------------------------------------------------*/
/*----- BTextView class ------------------------------------------*/

class BTextView : public BView {
public:
						BTextView(BRect			frame,
								  const char	*name,
								  BRect			textRect,
								  uint32		resizeMask,
								  uint32		flags = B_WILL_DRAW | B_PULSE_NEEDED);
						BTextView(BRect				frame, 
								  const char		*name, 
								  BRect				textRect,
						 	 	  const BFont		*initialFont,
								  const rgb_color	*initialColor, 
								  uint32			resizeMask, 
								  uint32			flags);
						BTextView(BMessage *data);
virtual					~BTextView();
	
static	BArchivable*	Instantiate(BMessage *data);
virtual	status_t		Archive(BMessage *data, bool deep = true) const;

virtual	void			AttachedToWindow();
virtual	void			DetachedFromWindow();
virtual	void			Draw(BRect inRect);
virtual	void			MouseDown(BPoint where);
virtual void			MouseUp(BPoint where);
virtual	void			MouseMoved(BPoint			where, 
								   uint32			code, 
							   	   const BMessage	*message);
virtual	void			WindowActivated(bool state);
virtual	void			KeyDown(const char *bytes, int32 numBytes);
virtual	void			Pulse();
virtual	void			FrameResized(float width, float height);
virtual	void			MakeFocus(bool focusState = true);
virtual	void			MessageReceived(BMessage *message);
virtual BHandler*		ResolveSpecifier(BMessage *message,
										 int32 index,
										 BMessage *specifier,
										 int32 form,
										 const char *property);
virtual status_t		GetSupportedSuites(BMessage *data);
virtual status_t		Perform(perform_code d, void *arg);
	
		void			SetText(const char *inText, 
								const text_run_array *inRuns = NULL);
		void			SetText(const char *inText, 
								int32 inLength,
								const text_run_array *inRuns = NULL);
		void			SetText(BFile *inFile,
								int32 startOffset,
								int32 inLength,
								const text_run_array *inRuns = NULL);

		void			Insert(const char *inText, 
							   const text_run_array *inRuns = NULL);
		void			Insert(const char *inText, 
							   int32 inLength,
							   const text_run_array *inRuns = NULL);
		void			Insert(int32 startOffset,
							   const char *inText,
							   int32 inLength,
							   const text_run_array *inRuns = NULL);

		void			Delete();
		void			Delete(int32 startOffset, int32 endOffset);
	
		const char*		Text() const;
		int32			TextLength() const;
		void			GetText(int32 offset, 
								int32 length,
								char *buffer) const;
		uchar			ByteAt(int32 offset) const;
	
		int32			CountLines() const;
		int32			CurrentLine() const;
		void			GoToLine(int32 lineNum);
	
virtual	void			Cut(BClipboard *clipboard);
virtual	void			Copy(BClipboard *clipboard);
virtual	void			Paste(BClipboard *clipboard);
		void			Clear();

virtual	bool			AcceptsPaste(BClipboard *clipboard);
virtual	bool			AcceptsDrop(const BMessage *inMessage);
			
virtual	void			Select(int32 startOffset, int32 endOffset);
		void			SelectAll();
		void			GetSelection(int32 *outStart, int32 *outEnd) const;

		void			SetFontAndColor(const BFont		*inFont, 
										uint32			inMode = B_FONT_ALL,
										const rgb_color	*inColor = NULL);
		void			SetFontAndColor(int32			startOffset, 
										int32			endOffset, 
										const BFont		*inFont,
										uint32			inMode = B_FONT_ALL,
										const rgb_color	*inColor = NULL);

		void			GetFontAndColor(int32		inOffset, 
										BFont		*outFont,
										rgb_color	*outColor = NULL) const;
		void			GetFontAndColor(BFont		*outFont,
										uint32		*outMode, 
										rgb_color	*outColor = NULL,
										bool		*outEqColor = NULL) const;

		void			SetRunArray(int32					startOffset, 
									int32					endOffset, 
									const text_run_array	*inRuns);
		text_run_array*	RunArray(int32	startOffset, 
								 int32	endOffset,
								 int32	*outSize = NULL) const;
	
		int32			LineAt(int32 offset) const;
		int32			LineAt(BPoint point) const;
		BPoint			PointAt(int32 inOffset, float *outHeight = NULL) const;
		int32			OffsetAt(BPoint point) const; 
		int32			OffsetAt(int32 line) const;

virtual	void			FindWord(int32	inOffset, 
								 int32	*outFromOffset, 
							 	 int32	*outToOffset);

virtual	bool			CanEndLine(int32 offset);
	
		float			LineWidth(int32 lineNum = 0) const;
		float			LineHeight(int32 lineNum = 0) const;
		float			TextHeight(int32 startLine, int32 endLine) const;
	
		void			GetTextRegion(int32		startOffset, 
									  int32		endOffset,
									  BRegion	*outRegion) const;
										
virtual	void			ScrollToOffset(int32 inOffset);
		void			ScrollToSelection();

		void			Highlight(int32 startOffset, int32 endOffset);	

		void			SetTextRect(BRect rect);
		BRect			TextRect() const;
		void			SetStylable(bool stylable);
		bool			IsStylable() const;
		void			SetTabWidth(float width);
		float			TabWidth() const;
		void			MakeSelectable(bool selectable = true);
		bool			IsSelectable() const;
		void			MakeEditable(bool editable = true);
		bool			IsEditable() const;
		void			SetWordWrap(bool wrap);
		bool			DoesWordWrap() const;
		void			SetMaxBytes(int32 max);
		int32			MaxBytes() const;
		void			DisallowChar(uint32 aChar);
		void			AllowChar(uint32 aChar);
		void			SetAlignment(alignment flag);
		alignment		Alignment() const;
		void			SetAutoindent(bool state);
		bool			DoesAutoindent() const;
		void			SetColorSpace(color_space colors);
		color_space		ColorSpace() const;
		void			MakeResizable(bool resize, BView *resizeView = NULL);
		bool			IsResizable() const;
		void			SetDoesUndo(bool undo);
		bool			DoesUndo() const;		
		void			HideTyping(bool enabled);
		bool			IsTypingHidden(void) const;	

virtual void			ResizeToPreferred();
virtual void			GetPreferredSize(float *width, float *height);
virtual void			AllAttached();
virtual void			AllDetached();


static	text_run_array*		AllocRunArray(int32 entryCount, int32 *outSize = NULL);
static	text_run_array* 	CopyRunArray(const text_run_array *orig, int32 countDelta = 0);
static	void			FreeRunArray(text_run_array *array);
static	void *			FlattenRunArray(const text_run_array *inArray, int32 *outSize = NULL);
static	text_run_array*		UnflattenRunArray(const void *data, int32 *outSize = NULL);
	
protected:
virtual	void			InsertText(const char				*inText, 
								   int32					inLength, 
								   int32					inOffset,
								   const text_run_array		*inRuns);
virtual	void			DeleteText(int32 fromOffset, int32 toOffset);

public:
virtual	void			Undo(BClipboard *clipboard);
		undo_state		UndoState(bool *isRedo) const;

protected:
virtual	void			GetDragParameters(BMessage	*drag, 
										  BBitmap	**bitmap,
										  BPoint	*point,
										  BHandler	**handler);
 
/*----- Private or reserved -----------------------------------------*/
private:
friend status_t	_init_interface_kit_();
friend class _BTextTrackState_;

#if _PR2_COMPATIBLE_
friend void		_ReservedTextView2__9BTextViewFv(BTextView	*object, 
												 BMessage	*drag, 
												 BBitmap	**bitmap, 
												 BPoint		*point, 
												 BHandler	**handler);
#endif

virtual void			_ReservedTextView3();
virtual void			_ReservedTextView4();
virtual void			_ReservedTextView5();
virtual void			_ReservedTextView6();
virtual void			_ReservedTextView7();
virtual void			_ReservedTextView8();

#if !_PR3_COMPATIBLE_
virtual void			_ReservedTextView9();
virtual void			_ReservedTextView10();
virtual void			_ReservedTextView11();
virtual void			_ReservedTextView12();
#endif

		void			InitObject(BRect			textRect, 
								   const BFont		*initialFont,
								   const rgb_color	*initialColor);

		void			HandleBackspace();
		void			HandleArrowKey(uint32 inArrowKey);
		void			HandleDelete();
		void			HandlePageKey(uint32 inPageKey);
		void			HandleAlphaKey(const char *bytes, int32 numBytes);
	
		void			Refresh(int32	fromOffset, 
								int32	toOffset, 
								bool	erase, 
								bool	scroll);
		void			RecalculateLineBreaks(int32 *startLine, int32 *endLine);
		int32			FindLineBreak(int32	fromOffset, 
									  float	*outAscent, 
								  	  float	*outDescent, 
									  float	*ioWidth);
	
		float			StyledWidth(int32	fromOffset, 
									int32	length,
									float	*outAscent = NULL, 
									float	*outDescent = NULL) const;
		float			StyledWidthUTF8Safe(int32 fromOffset,
											int32 numChars,
											float	*outAscent = NULL, 
											float	*outDescent = NULL) const;
		
		float			ActualTabWidth(float location) const;
		
		void			DoInsertText(const char				*inText, 
									 int32					inLength, 
									 int32					inOffset,
									 const text_run_array	*inRuns,
									 _BTextChangeResult_	*outResult);
		void			DoDeleteText(int32					fromOffset,
									 int32					toOffset,
									 _BTextChangeResult_	*outResult);
		
		void			DrawLines(int32	startLine, 
								  int32	endLine, 
							  	  int32	startOffset = -1, 
								  bool	erase = false);
		void			DrawCaret(int32 offset);
		void			InvertCaret();
		void			DragCaret(int32 offset);
		
		void			StopMouseTracking();
		bool			PerformMouseUp(BPoint where);
		bool			PerformMouseMoved(BPoint			where, 
										  uint32			code);
		
		void			TrackMouse(BPoint where, const BMessage *message,
								   bool force = false);

		void			TrackDrag(BPoint where);
		void			InitiateDrag();
		bool			MessageDropped(BMessage	*inMessage, 
									   BPoint	where,
									   BPoint	offset);
									   
		void			PerformAutoScrolling();									
		void			UpdateScrollbars();
		void			AutoResize(bool doredraw=true);
		
		void			NewOffscreen(float padding = 0.0F);
		void			DeleteOffscreen();

		void			Activate();
		void			Deactivate();

		void			NormalizeFont(BFont *font);

		uint32			CharClassification(int32 offset) const;
		int32			NextInitialByte(int32 offset) const;
		int32			PreviousInitialByte(int32 offset) const;

		bool			GetProperty(BMessage	*specifier,
									int32		form, 
									const char	*property,
									BMessage	*reply);
		bool			SetProperty(BMessage	*specifier,
									int32		form, 
									const char	*property,
									BMessage	*reply);
		bool			CountProperties(BMessage	*specifier,
										int32		form,
										const char	*property,
										BMessage	*reply);

		void			HandleInputMethodChanged(BMessage *message);
		void			HandleInputMethodLocationRequest();
		void			CancelInputMethod();

static	void			LockWidthBuffer();
static	void			UnlockWidthBuffer();	
		
		_BTextGapBuffer_*		fText;
		_BLineBuffer_*			fLines;
		_BStyleBuffer_*			fStyles;
		BRect					fTextRect;
		int32					fSelStart;
		int32					fSelEnd;
		bool					fCaretVisible;
		bigtime_t				fCaretTime;
		int32					fClickOffset;
		int32					fClickCount;
		bigtime_t				fClickTime;
		int32					fDragOffset;
		uint8					fCursor;
		bool					fActive;
		bool					fStylable;
		float					fTabWidth;
		bool					fSelectable;
		bool					fEditable;
		bool					fWrap;
		int32					fMaxBytes; 
		BList*					fDisallowedChars;
		alignment				fAlignment;
		bool					fAutoindent;
		BBitmap* 				fOffscreen;
		color_space				fColorSpace;
		bool					fResizable;
		BView*					fContainerView;
		_BUndoBuffer_*			fUndo;			/* was _reserved[0] */
		_BInlineInput_*			fInline;		/* was _reserved[1] */
		BMessageRunner *		fDragRunner;	/* was _reserved[2] */
		BMessageRunner *		fClickRunner;	/* was _reserved[3] */
		BPoint					fWhere;
		_BTextTrackState_*		fTrackingMouse;	/* was _reserved[6] */
		_BTextChangeResult_*	fTextChange;	/* was _reserved[7] */
		uint32					_reserved[1];	/* was 8 */
#if !_PR3_COMPATIBLE_
		uint32					_more_reserved[8];
#endif

static	_BWidthBuffer_*			sWidths;
static	sem_id					sWidthSem;
static	int32					sWidthAtom;
};	

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _TEXT_VIEW_H */
