/*****************************************************************************/
// ShowImageView
// Written by Fernando Francisco de Oliveira, Michael Wilber, Michael Pfeiffer
//
// ShowImageView.h
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef _ShowImageView_h
#define _ShowImageView_h

#include <View.h>
#include <Bitmap.h>
#include <Entry.h>
#include <NodeInfo.h>
#include <String.h>
#include <TranslatorRoster.h>

#include "Filter.h"
#include "ShowImageUndo.h"

#define DELAYED_SCALING 1

class ShowImageView : public BView {
public:
	ShowImageView(BRect rect, const char *name, uint32 resizingMode,
		uint32 flags);
	~ShowImageView();
	
	void Pulse();
	
	status_t SetImage(const entry_ref *pref);
	void SetDither(bool dither);
	bool GetDither() const { return fDither; }
	void SetShowCaption(bool show);
	void SetShrinkToBounds(bool enable);
	bool GetShrinkToBounds() const { return fShrinkToBounds; }
	void SetZoomToBounds(bool enable);
	bool GetZoomToBounds() const { return fZoomToBounds; }
	void SetBorder(bool hasBorder);
	bool HasBorder() const { return fHasBorder; }
	void SetAlignment(alignment horizontal, vertical_alignment vertical);
	BBitmap *GetBitmap();
	void GetName(BString *name);
	void GetPath(BString *name);
	void FlushToLeftTop();
	void SetScaleBilinear(bool b);
	bool GetScaleBilinear() { return fScaleBilinear; }
	
	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void FrameResized(float width, float height);
	virtual void MouseDown(BPoint point);
	virtual void MouseMoved(BPoint point, uint32 state, const BMessage *pmsg);
	virtual void MouseUp(BPoint point);
	virtual void KeyDown(const char *bytes, int32 numBytes);

	virtual void MessageReceived(BMessage *pmsg);
	
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
	void Mirror(bool vertical);
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
	void PasteBitmap(BBitmap *bitmap, BPoint point);
	void MergeWithBitmap(BBitmap *merge, BRect selection);
	void MergeSelection();
	void DeleteScaler();
	void DeleteBitmap();
	void DeleteSelBitmap();
	int32 BytesPerPixel(color_space cs) const;
	inline void CopyPixel(uchar* dest, int32 destX, int32 destY, int32 destBPR, uchar* src, int32 x, int32 y, int32 bpr, int32 bpp);
	inline void InvertPixel(int32 x, int32 y, uchar* dest, int32 destBPR, uchar* src, int32 bpr, int32 bpp);
	void DoImageOperation(enum ImageProcessor::operation op, bool quiet = false);
	BRect AlignBitmap();
	void Setup(BRect r);
	BPoint ImageToView(BPoint p) const;
	BPoint ViewToImage(BPoint p) const;
	BRect ImageToView(BRect r) const;
	bool IsImage(const entry_ref* pref);
	static int CompareEntries(const void* a, const void* b);
	void FreeEntries(BList* entries);
	bool FindNextImage(entry_ref *in_current, entry_ref *out_image, bool next, bool rewind);
	bool ShowNextImage(bool next, bool rewind);
	bool FirstFile();
	void ConstrainToImage(BPoint &point);
	void ConstrainToImage(BRect &rect);
	BBitmap* CopyFromRect(BRect srcRect);
	BBitmap* CopySelection(uchar alpha = 255, bool imageSize = true);
	bool AddSupportedTypes(BMessage* msg, BBitmap* bitmap);
	void BeginDrag(BPoint sourcePoint);
	void SaveToFile(BDirectory* dir, const char* name, BBitmap* bitmap, translation_format* format);
	void SendInMessage(BMessage* msg, BBitmap* bitmap, translation_format* format);
	bool OutputFormatForType(BBitmap* bitmap, const char* type, translation_format* format);
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
	
	entry_ref fCurrentRef; // of the image
	bool fDither;          // dither the image
	int32 fDocumentIndex;  // of the image in the file
	int32 fDocumentCount;  // number of images in the file
	BBitmap *fBitmap;      // to be displayed
	BBitmap *fSelBitmap;   // the bitmap in the selection
	float fZoom;           // factor to be used to display the image
	bool fScaleBilinear;   // use bilinear scaling?
	Scaler* fScaler;       // holds the scaled image if bilinear scaling is enabled
	bool fShrinkToBounds;  // shrink images to view bounds that are larger than the view
	bool fZoomToBounds;    // zoom images to view bounds that are smaller than the view
	bool fShrinkOrZoomToBounds;
	bool fHasBorder;       // should the image have a border?
	alignment fHAlignment; // horizontal alignment (left and centered only)
	vertical_alignment fVAlignment; // vertical alignment (left and centered only)
	float fLeft;         // the origin of the image in the view
	float fTop;
	float fScaleX;       // to convert image from/to view coordinates
	float fScaleY;
	bool fMovesImage;     // is the image being moved with the mouse
	bool fMakesSelection; // is a selection being made
	BPoint fFirstPoint;   // first point in image space of selection
	bool fAnimateSelection; // marching ants
	bool fHasSelection;  // is fSelectionRect valid 
	BRect fSelectionRect; // the current location of the selection rectangle
	BRect fCopyFromRect;
		// the portion of the background bitmap the selection is made from
	pattern fPatternUp, fPatternDown, fPatternLeft, fPatternRight;
	
	bool fSlideShow; // is slide show enabled?
	int fSlideShowDelay; // in pulse rate units
	int fSlideShowCountDown; // shows next image if it reaches zero
#if DELAYED_SCALING
	int fScalingCountDown;
#endif
	
	bool fShowCaption; // display caption?
	BString fCaption; // caption text

	bool fInverted;
	enum image_orientation fImageOrientation;
	static enum image_orientation fTransformation[ImageProcessor::kNumberOfAffineTransformations][kNumberOfOrientations];
};

#endif /* _ShowImageView_h */
