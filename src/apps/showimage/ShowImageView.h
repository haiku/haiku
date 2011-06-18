/*
 * Copyright 2003-2011, Haiku, Inc. All Rights Reserved.
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


#include <Bitmap.h>
#include <Entry.h>
#include <NodeInfo.h>
#include <String.h>
#include <TranslatorRoster.h>
#include <View.h>

#include "Filter.h"
#include "SelectionBox.h"
#include "ShowImageUndo.h"


class BitmapOwner;


class ShowImageView : public BView {
public:
								ShowImageView(BRect rect, const char* name,
									uint32 resizingMode, uint32 flags);
	virtual						~ShowImageView();

	virtual	void				AttachedToWindow();
	virtual void				FrameResized(float width, float height);
	virtual	void				Draw(BRect updateRect);
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
			status_t			SetImage(const BMessage* message);
			status_t			SetImage(const entry_ref* ref, BBitmap* bitmap);
			const entry_ref*	Image() const { return &fCurrentRef; }
			BBitmap*			Bitmap();

			BPoint				ImageToView(BPoint p) const;
			BPoint				ViewToImage(BPoint p) const;
			BRect				ImageToView(BRect r) const;
			void				ConstrainToImage(BPoint& point) const;
			void				ConstrainToImage(BRect& rect) const;

			void				SaveToFile(BDirectory* dir, const char* name,
									BBitmap* bitmap,
									const translation_format* format);

			void				SetScaleBilinear(bool b);
			bool				ScaleBilinear() { return fScaleBilinear; }
			void				SetShowCaption(bool show);
			void				SetStretchToBounds(bool enable);
			bool				StretchesToBounds() const
									{ return fStretchToBounds; }
			void				SetHideIdlingCursor(bool hide);

			void				FixupScrollBar(enum orientation orientation,
									float bitmapLength, float viewLength);
			void				FixupScrollBars();

			void				SetSelectionMode(bool selectionMode);
			bool				IsSelectionModeEnabled() const
									{ return fSelectionMode; }
			void				Undo();
			void				SelectAll();
			void				ClearSelection();

			void				CopySelectionToClipboard();

			void				FitToBounds();
			void				SetZoom(float zoom,
									BPoint where = BPoint(-1, -1));
			float				Zoom() const
									{ return fZoom; }
			void				ZoomIn(BPoint where = BPoint(-1, -1));
			void				ZoomOut(BPoint where = BPoint(-1, -1));

			// Image manipulation
			void				Rotate(int degree); // 90 and 270 only
			void				Flip(bool vertical);
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

			void				_SetHasSelection(bool bHasSelection);
			void				_AnimateSelection(bool a);
			void				_SendMessageToWindow(BMessage *message);
			void				_SendMessageToWindow(uint32 code);
			void				_Notify();
			void				_UpdateStatusText();
			void				_GetMergeRects(BBitmap* merge,
									BRect selection, BRect& srcRect,
									BRect& dstRect);
			void				_GetSelectionMergeRects(BRect& srcRect,
									BRect& dstRect);
			status_t			_SetSelection(const entry_ref* ref,
									BPoint point);
			void				_MergeWithBitmap(BBitmap* merge,
									BRect selection);
			void				_MergeSelection();
			void				_DeleteScaler();
			void				_DeleteBitmap();
			void				_DeleteSelectionBitmap();

			void				_DoImageOperation(
									enum ImageProcessor::operation op,
									bool quiet = false);
			void				_UserDoImageOperation(
									enum ImageProcessor::operation op,
									bool quiet = false);
			bool				_ShouldStretch() const;
			float				_FitToBoundsZoom() const;
			BRect				_AlignBitmap();
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
			void				_ScrollBitmap(BPoint point);
			void				_UpdateSelectionRect(BPoint point, bool final);
			void				_DrawBackground(BRect aroundFrame);
			void				_LayoutCaption(BFont& font, BPoint& textPos,
									BRect& background);
			void				_DrawCaption();
			void				_UpdateCaption();

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
			void				_StopSlideShow();
			void				_ExitFullScreen();
			void				_ShowToolBarIfEnabled(bool show);

private:
			ShowImageUndo		fUndo;
			entry_ref			fCurrentRef;

			BitmapOwner*		fBitmapOwner;
			BBitmap*			fBitmap;
			BBitmap*			fDisplayBitmap;
			BBitmap*			fSelectionBitmap;

			float				fZoom;

			bool				fScaleBilinear;

			BPoint				fBitmapLocationInView;

			bool				fStretchToBounds;
			bool				fHideCursor;
			bool				fScrollingBitmap;
			bool				fCreatingSelection;
			BPoint				fFirstPoint;
				// first point in image space of selection
			bool				fSelectionMode;
			bool				fAnimateSelection;
			bool				fHasSelection;
			SelectionBox		fSelectionBox;
			BRect				fCopyFromRect;
				// the portion of the background bitmap the selection is made
				// from

			bool				fShowCaption;
			BString				fCaption;

			BString				fFormatDescription;
			BString				fMimeType;

			bool				fShowingPopUpMenu;

			int					fHideCursorCountDown;
				// Hides the cursor when it reaches zero
			bool				fIsActiveWin;
				// Is the parent window the active window?

			BCursor*			fDefaultCursor;
			BCursor*			fGrabCursor;

			image_orientation	fImageOrientation;
	static	image_orientation	fTransformation[
									ImageProcessor
										::kNumberOfAffineTransformations]
									[kNumberOfOrientations];
};

#endif	// SHOW_IMAGE_VIEW_H
