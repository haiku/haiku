/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 */
#ifndef SHOW_IMAGE_VIEW_H
#define SHOW_IMAGE_VIEW_H


#include "Filter.h"
#include "ShowImageUndo.h"

#include <Bitmap.h>
#include <Entry.h>
#include <NodeInfo.h>
#include <String.h>
#include <TranslatorRoster.h>
#include <View.h>


// delay scaling operation, so that a sequence of zoom in/out operations works smoother
#define DELAYED_SCALING 1
// width of the black border stroked arround the bitmap
#define PEN_SIZE 1.0f

class ShowImageView : public BView {
	public:
		ShowImageView(BRect rect, const char *name, uint32 resizingMode,
			uint32 flags);
		virtual ~ShowImageView();

		virtual void AttachedToWindow();
		virtual void Draw(BRect updateRect);
		virtual void FrameResized(float width, float height);
		virtual void MouseDown(BPoint point);
		virtual void MouseMoved(BPoint point, uint32 state, const BMessage *message);
		virtual void MouseUp(BPoint point);
		virtual void KeyDown(const char *bytes, int32 numBytes);
		virtual void Pulse();

		virtual void MessageReceived(BMessage *message);

		void SetTrackerMessenger(const BMessenger& trackerMessenger);
		status_t SetImage(const entry_ref *ref);
		const entry_ref* Image() const { return &fCurrentRef; }

		void SaveToFile(BDirectory* dir, const char* name, BBitmap* bitmap,
				const translation_format* format);
		void SetDither(bool dither);
		bool GetDither() const { return fDither; }
		void SetShowCaption(bool show);
		void SetShrinkToBounds(bool enable);
		bool GetShrinkToBounds() const { return fShrinkToBounds; }
		void SetZoomToBounds(bool enable);
		bool GetZoomToBounds() const { return fZoomToBounds; }
		void SetFullScreen(bool fullScreen);
		BBitmap *GetBitmap();
		void GetName(BString *name);
		void GetPath(BString *name);
		void SetScaleBilinear(bool b);
		bool GetScaleBilinear() { return fScaleBilinear; }

		void FixupScrollBar(orientation o, float bitmapLength, float viewLength);
		void FixupScrollBars();

		int32 CurrentPage();
		int32 PageCount();

		void Undo();
		void Cut();
		void Paste();
		void SelectAll();
		void ClearSelection();

		void CopySelectionToClipboard();

		void FirstPage();
		void LastPage();
		void NextPage();
		void PrevPage();
		void GoToPage(int32 page);
		bool NextFile();
		bool PrevFile();
		bool HasNextFile();
		bool HasPrevFile();
		void SetSlideShowDelay(float seconds);
		float GetSlideShowDelay() const { return fSlideShowDelay / 10.0; }
		bool SlideShowStarted() const { return fSlideShow; }
		void StartSlideShow();
		void StopSlideShow();
		void SetZoom(float zoom);
		void ZoomIn();
		void ZoomOut();

		// Image manipulation
		void Rotate(int degree); // 90 and 270 only
		void Flip(bool vertical);
		void Invert();

		void SetIcon(bool clear);

	private:
		ShowImageUndo fUndo;
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
		void InitPatterns();
		void RotatePatterns();
		void RemoveSelection(bool bToClipboard);
		bool HasSelection() { return fHasSelection; }
		void SetHasSelection(bool bHasSelection);
		void AnimateSelection(bool a);
		void Notify(const char* status);
		void AddToRecentDocuments();
		void AddWhiteRect(BRect &rect);
		void GetMergeRects(BBitmap *merge, BRect selection, BRect &srcBits, BRect &destRect);
		void GetSelMergeRects(BRect &srcBits, BRect &destRect);
		status_t SetSelection(const entry_ref *pref, BPoint point);
		status_t PasteBitmap(BBitmap *bitmap, BPoint point);
		void MergeWithBitmap(BBitmap *merge, BRect selection);
		void MergeSelection();
		void DeleteScaler();
		void DeleteBitmap();
		void DeleteSelBitmap();
		int32 BytesPerPixel(color_space cs) const;
		void CopyPixel(uchar* dest, int32 destX, int32 destY, int32 destBPR,
				uchar* src, int32 x, int32 y, int32 bpr, int32 bpp);
		void InvertPixel(int32 x, int32 y, uchar* dest, int32 destBPR, uchar* src,
				int32 bpr, int32 bpp);
		void DoImageOperation(enum ImageProcessor::operation op, bool quiet = false);
		void UserDoImageOperation(enum ImageProcessor::operation op, bool quiet = false);
		BRect AlignBitmap();
		void Setup(BRect r);
		BPoint ImageToView(BPoint p) const;
		BPoint ViewToImage(BPoint p) const;
		BRect ImageToView(BRect r) const;
		bool IsImage(const entry_ref* pref);
		static int CompareEntries(const void* a, const void* b);
		void FreeEntries(BList* entries);
		void SetTrackerSelectionToCurrent();
		bool FindNextImageByDir(entry_ref *in_current, entry_ref *out_image,
				bool next, bool rewind);
		bool FindNextImage(entry_ref *in_current, entry_ref *out_image,
				bool next, bool rewind);
		bool ShowNextImage(bool next, bool rewind);
		bool FirstFile();
		void ConstrainToImage(BPoint &point);
		void ConstrainToImage(BRect &rect);
		BBitmap* CopyFromRect(BRect srcRect);
		BBitmap* CopySelection(uchar alpha = 255, bool imageSize = true);
		bool AddSupportedTypes(BMessage* msg, BBitmap* bitmap);
		void BeginDrag(BPoint sourcePoint);
		void SendInMessage(BMessage* msg, BBitmap* bitmap, translation_format* format);
		bool OutputFormatForType(BBitmap* bitmap, const char* type,
				translation_format* format);
		void HandleDrop(BMessage* msg);
		void MoveImage();
		uint32 GetMouseButtons();
		void UpdateSelectionRect(BPoint point, bool final);
		void DrawBorder(BRect border);
		void LayoutCaption(BFont &font, BPoint &textPos, BRect &background);
		void DrawCaption();
		void UpdateCaption();
		void DrawSelectionBox();
		Scaler* GetScaler(BRect rect);
		void DrawImage(BRect rect);
		float LimitToRange(float v, orientation o, bool absolute);
		void ScrollRestricted(float x, float y, bool absolute);
		void ScrollRestrictedTo(float x, float y);
		void ScrollRestrictedBy(float x, float y);
		void MouseWheelChanged(BMessage* msg);
		void ShowPopUpMenu(BPoint screen);
		void SettingsSetBool(const char* name, bool value);
		void SetIcon(bool clear, icon_size which);
		void ToggleSlideShow();
		void ExitFullScreen();

		BMessenger fTrackerMessenger; // of the window that this was launched from
		entry_ref fCurrentRef;		// of the image
		bool fDither;				// dither the image
		int32 fDocumentIndex;		// of the image in the file
		int32 fDocumentCount;		// number of images in the file
		BBitmap *fBitmap;			// the original image
		BBitmap *fDisplayBitmap;
			// the image to be displayed
			// (== fBitmap if the bitmap can be displayed as is)
		BBitmap *fSelBitmap;		// the bitmap in the selection
		float fZoom;				// factor to be used to display the image
		bool fScaleBilinear;		// use bilinear scaling?
		Scaler* fScaler;			// holds the scaled image if bilinear scaling is enabled
		bool fShrinkToBounds;
		bool fZoomToBounds;
		bool fShrinkOrZoomToBounds;
		bool fFullScreen;			// is the image displayed fullscreen?
		float fLeft;				// the origin of the image in the view
		float fTop;
		bool fMovesImage;			// is the image being moved with the mouse
		bool fMakesSelection;		// is a selection being made
		BPoint fFirstPoint;			// first point in image space of selection
		bool fAnimateSelection;		// marching ants
		bool fHasSelection;			// is fSelectionRect valid 
		BRect fSelectionRect;		// the current location of the selection rectangle
		BRect fCopyFromRect;
			// the portion of the background bitmap the selection is made from
		pattern fPatternUp, fPatternDown, fPatternLeft, fPatternRight;
	
		bool fSlideShow;			// is slide show enabled?
		int fSlideShowDelay;		// in pulse rate units
		int fSlideShowCountDown;	// shows next image if it reaches zero
#if DELAYED_SCALING
		int fScalingCountDown;
#endif
	
		bool fShowCaption;			// display caption?
		BString fCaption;			// caption text

		bool fInverted;

		bool fShowingPopUpMenu;

		enum image_orientation fImageOrientation;
		static enum image_orientation fTransformation[
			ImageProcessor::kNumberOfAffineTransformations][kNumberOfOrientations];
};

#endif	// SHOW_IMAGE_VIEW_H
