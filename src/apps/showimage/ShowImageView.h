/*
 * Copyright 2003-2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2004-2005 yellowTAB GmbH. All Rights Reserverd.
 * Copyright 2006 Bernd Korz. All Rights Reserved
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 *		yellowTAB GmbH
 *		Bernd Korz
 */
#ifndef SHOW_IMAGE_VIEW_H
#define SHOW_IMAGE_VIEW_H


#include "Filter.h"
#include "SelectionBox.h"
#include "ShowImageUndo.h"

#include <Bitmap.h>
#include <Entry.h>
#include <NodeInfo.h>
#include <String.h>
#include <TranslatorRoster.h>
#include <View.h>


// Delay scaling operation, so that a sequence of zoom in/out operations works
// smoother
#define DELAYED_SCALING 1
// the delay time in 1/10 seconds
#define SCALING_DELAY_TIME 3
// width of the black border stroked arround the bitmap
#define PEN_SIZE 1.0f

// the delay time for hiding the cursor in 1/10 seconds (the pulse rate)
#define HIDE_CURSOR_DELAY_TIME 20

class ProgressWindow;

class ShowImageView : public BView {
public:
								ShowImageView(BRect rect, const char* name,
									uint32 resizingMode, uint32 flags);
	virtual						~ShowImageView();

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);
	virtual	void				MouseDown(BPoint point);
	virtual	void				MouseMoved(BPoint point, uint32 state,
									const BMessage* dragMessage);
	virtual	void				MouseUp(BPoint point);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				Pulse();

	virtual void				MessageReceived(BMessage* message);
	virtual void				WindowActivated(bool active);

			void				SetTrackerMessenger(
									const BMessenger& trackerMessenger);
			status_t			SetImage(const entry_ref* ref);
			const entry_ref*	Image() const { return &fCurrentRef; }

			BPoint				ImageToView(BPoint p) const;
			BPoint				ViewToImage(BPoint p) const;
			BRect				ImageToView(BRect r) const;
			void				ConstrainToImage(BPoint& point) const;
			void				ConstrainToImage(BRect& rect) const;

			void				SaveToFile(BDirectory* dir, const char* name,
									BBitmap* bitmap,
									const translation_format* format);
			void				SetDither(bool dither);
			bool				GetDither() const { return fDither; }
			void				SetScaleBilinear(bool b);
			bool				GetScaleBilinear() { return fScaleBilinear; }
			void				SetShowCaption(bool show);
			void				SetShrinkToBounds(bool enable);
			bool				GetShrinkToBounds() const
									{ return fShrinkToBounds; }
			void				SetZoomToBounds(bool enable);
			bool				GetZoomToBounds() const
									{ return fZoomToBounds; }
			void				SetFullScreen(bool fullScreen);

			BBitmap*			GetBitmap();
			void				GetName(BString* name);
			void				GetPath(BString* name);

			void				FixupScrollBar(enum orientation orientation,
									float bitmapLength, float viewLength);
			void				FixupScrollBars();

			void				Undo();
			void				Cut();
			void				Paste();
			void				SelectAll();
			void				ClearSelection();

			void				CopySelectionToClipboard();

			int32				CurrentPage();
			int32				PageCount();

			void				FirstPage();
			void				LastPage();
			void				NextPage();
			void				PrevPage();
			void				GoToPage(int32 page);

			bool				NextFile();
			bool				PrevFile();
			bool				HasNextFile();
			bool				HasPrevFile();

			void				SetSlideShowDelay(float seconds);
			float				GetSlideShowDelay() const
									{ return fSlideShowDelay / 10.0; }
			bool				SlideShowStarted() const { return fSlideShow; }
			void				StartSlideShow();
			void				StopSlideShow();
			void				SetZoom(float zoom);
			void				ZoomIn();
			void				ZoomOut();

			// Image manipulation
			void				Rotate(int degree); // 90 and 270 only
			void				Flip(bool vertical);
			void				Invert();
			void				ResizeImage(int width, int height);

			void				SetIcon(bool clear);

private:
			enum image_orientation {
				k0,    // 0
				k90,   // 1
				k180,  // 2
				k270,  // 3
				k0V,   // 4
				k90V,  // 5
				k0H,   // 6
				k270V, // 7
				kNumberOfOrientations,
			};

			void				_RemoveSelection(bool bToClipboard,
									bool neverCutBackground = false);
			void				_SetHasSelection(bool bHasSelection);
			void				_AnimateSelection(bool a);
			void				_SendMessageToWindow(BMessage *message);
			void				_SendMessageToWindow(uint32 code);
			void				_Notify();
			void				_UpdateStatusText();
			void				_AddToRecentDocuments();
			void				_AddWhiteRect(BRect& rect);
			void				_GetMergeRects(BBitmap* merge,
									BRect selection, BRect& srcRect,
									BRect& dstRect);
			void				_GetSelectionMergeRects(BRect& srcRect,
									BRect& dstRect);
			status_t			_SetSelection(const entry_ref* ref,
									BPoint point);
			status_t			_PasteBitmap(BBitmap* bitmap, BPoint point);
			void				_MergeWithBitmap(BBitmap* merge,
									BRect selection);
			void				_MergeSelection();
			void				_DeleteScaler();
			void				_DeleteBitmap();
			void				_DeleteSelectionBitmap();

			int32				_BytesPerPixel(color_space cs) const;
			void				_CopyPixel(uchar* dest, int32 destX,
									int32 destY, int32 destBPR, uchar* src,
									int32 x, int32 y, int32 bpr, int32 bpp);
			void				_InvertPixel(int32 x, int32 y, uchar* dest,
									int32 destBPR, uchar* src, int32 bpr,
									int32 bpp);
			void				_DoImageOperation(
									enum ImageProcessor::operation op,
									bool quiet = false);
			void				_UserDoImageOperation(
									enum ImageProcessor::operation op,
									bool quiet = false);
			BRect				_AlignBitmap();
			void				_Setup(BRect r);
			bool				_IsImage(const entry_ref* pref);
	static	int					_CompareEntries(const void* a, const void* b);
			void				_FreeEntries(BList* entries);
			void				_SetTrackerSelectionToCurrent();
			bool				_FindNextImage(entry_ref* inCurrent,
									entry_ref* outImage, bool next,
									bool rewind);
			bool				_ShowNextImage(bool next, bool rewind);
			bool				_FirstFile();
			BBitmap*			_CopyFromRect(BRect srcRect);
			BBitmap*			_CopySelection(uchar alpha = 255,
									bool imageSize = true);
			bool				_AddSupportedTypes(BMessage* message,
									BBitmap* bitmap);
			void				_BeginDrag(BPoint sourcePoint);
			void				_SendInMessage(BMessage* message,
									BBitmap* bitmap,
									translation_format* format);
			bool				_OutputFormatForType(BBitmap* bitmap,
									const char* type,
									translation_format* format);
			void				_HandleDrop(BMessage* message);
			void				_ScrollBitmap();
			uint32				_GetMouseButtons();
			void				_UpdateSelectionRect(BPoint point, bool final);
			void				_DrawBorder(BRect border);
			void				_LayoutCaption(BFont& font, BPoint& textPos,
									BRect& background);
			void				_DrawCaption();
			void				_UpdateCaption();

			Scaler*				_GetScaler(BRect rect);
			void				_DrawImage(BRect rect);
			float				_LimitToRange(float v, orientation o,
									bool absolute);
			void				_ScrollRestricted(float x, float y,
									bool absolute);
			void				_ScrollRestrictedTo(float x, float y);
			void				_ScrollRestrictedBy(float x, float y);
			void				_MouseWheelChanged(BMessage* message);
			void				_ShowPopUpMenu(BPoint screen);
			void				_SettingsSetBool(const char* name, bool value);
			void				_SetIcon(bool clear, icon_size which);
			void				_ToggleSlideShow();
			void				_ExitFullScreen();

private:
			ShowImageUndo		fUndo;
			BMessenger			fTrackerMessenger;
				// of the window that this was launched from
			entry_ref			fCurrentRef;

			int32				fDocumentIndex;
				// of the image in the file
			int32				fDocumentCount;
				// number of images in the file

			BBitmap*			fBitmap;
			BBitmap*			fDisplayBitmap;
			BBitmap*			fSelectionBitmap;

			float				fZoom;

			bool				fDither;
			bool				fScaleBilinear;
			Scaler*				fScaler;
#if DELAYED_SCALING
			int					fScalingCountDown;
#endif
			bool				fInverted;

			BPoint				fBitmapLocationInView;

			bool				fShrinkToBounds;
			bool				fZoomToBounds;
			bool				fShrinkOrZoomToBounds;
			bool				fFullScreen;
			bool				fScrollingBitmap;
			bool				fCreatingSelection;
			BPoint				fFirstPoint;
				// first point in image space of selection
			bool				fAnimateSelection;
			bool				fHasSelection;
			SelectionBox		fSelectionBox;
			BRect				fCopyFromRect;
				// the portion of the background bitmap the selection is made
				// from

			bool				fSlideShow;
			int					fSlideShowDelay;
				// in pulse rate units
			int					fSlideShowCountDown;
				// shows next image if it reaches zero
	
			bool				fShowCaption;
			BString				fCaption;

			BString				fImageType;
				// Type of image, for use in status bar and caption
			BString				fImageMime;

			bool				fShowingPopUpMenu;

			int					fHideCursorCountDown;
				// Hides the cursor when it reaches zero
			bool				fIsActiveWin;
				// Is the parent window the active window?

			ProgressWindow*		fProgressWindow;

			image_orientation	fImageOrientation;
	static	image_orientation	fTransformation[
									ImageProcessor
										::kNumberOfAffineTransformations]
									[kNumberOfOrientations];
};

#endif	// SHOW_IMAGE_VIEW_H
