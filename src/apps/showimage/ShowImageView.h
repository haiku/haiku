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
#include <String.h>
#include <TranslatorRoster.h>

#include "Filter.h"

class ShowImageView : public BView {
public:
	ShowImageView(BRect rect, const char *name, uint32 resizingMode,
		uint32 flags);
	~ShowImageView();
	
	void Pulse();
	
	void SetImage(const entry_ref *pref);
	void SetShowCaption(bool show);
	void ResizeToViewBounds(bool resize);
	bool GetResizeToViewBounds() const { return fResizeToViewBounds; }
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
	
	void FixupScrollBars();
	
	int32 CurrentPage();
	int32 PageCount();
	
	void SelectAll();
	void Unselect();
	
	void CopySelectionToClipboard();
	
	void FirstPage();
	void LastPage();
	void NextPage();
	void PrevPage();
	void GoToPage(int32 page);
	bool NextFile();
	bool PrevFile();
	void SetSlideShowDelay(float seconds);
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
	
private:
	enum image_operation {
		kRotateClockwise,
		kRotateAntiClockwise,
		kMirrorVertical,
		kMirrorHorizontal,
		kInvert
	};
	void InitPatterns();
	void RotatePatterns();
	void AnimateSelection(bool a);
	void Notify(const char* status);
	void AddToRecentDocuments();
	void DeleteScaler();
	void DeleteBitmap();
	int32 BytesPerPixel(color_space cs) const;
	inline void CopyPixel(uchar* dest, int32 destX, int32 destY, int32 destBPR, uchar* src, int32 x, int32 y, int32 bpr, int32 bpp);
	inline void InvertPixel(int32 x, int32 y, uchar* dest, int32 destBPR, uchar* src, int32 bpr, int32 bpp);
	void DoImageOperation(image_operation op);
	BRect AlignBitmap();
	void Setup(BRect r);
	BPoint ImageToView(BPoint p) const;
	BPoint ViewToImage(BPoint p) const;
	BRect ImageToView(BRect r) const;
	bool IsImage(const entry_ref* pref);
	static int CompareEntries(const void* a, const void* b);
	void FreeEntries(BList* entries);
	bool FindNextImage(entry_ref* ref, bool next, bool rewind);
	bool ShowNextImage(bool next, bool rewind);
	bool FirstFile();
	void ConstrainToImage(BPoint &point);
	BBitmap* CopySelection(uchar alpha = 255);
	bool AddSupportedTypes(BMessage* msg, BBitmap* bitmap);
	void BeginDrag(BPoint point);
	void SaveToFile(BDirectory* dir, const char* name, BBitmap* bitmap, translation_format* format);
	void SendInMessage(BMessage* msg, BBitmap* bitmap, translation_format* format);
	bool OutputFormatForType(BBitmap* bitmap, const char* type, translation_format* format);
	void HandleDrop(BMessage* msg);
	void MoveImage();
	uint32 GetMouseButtons();
	void UpdateSelectionRect(BPoint point, bool final);
	void DrawBorder(BRect border);
	void DrawCaption();
	void EraseCaption();
	void DrawSelectionBox(BRect &rect);
	Scaler* GetScaler();
	void DrawImage(BRect rect);
	float LimitToRange(float v, orientation o, bool absolute);
	void ScrollRestricted(float x, float y, bool absolute);
	void ScrollRestrictedTo(float x, float y);
	void ScrollRestrictedBy(float x, float y);
	void MouseWheelChanged(BMessage* msg);
	void ShowPopUpMenu(BPoint screen);
	
	entry_ref fCurrentRef;
	int32 fDocumentIndex;
	int32 fDocumentCount;
	BBitmap *fpBitmap;
	float fZoom;
	bool fScaleBilinear;
	Scaler* fScaler;
	bool fResizeToViewBounds;
	alignment fHAlignment;
	vertical_alignment fVAlignment;
	float fLeft;         // the origin of the image in the view
	float fTop;
	float fScaleX;
	float fScaleY;
	bool fMovesImage;
	bool fBeginDrag;
	bool fMakesSelection; // is a selection being made
	BPoint fFirstPoint;   // first point in image space of selection
	bool fAnimateSelection; // marching ants
	bool fbHasSelection;  // is fSelectionRect valid 
	BRect fSelectionRect; // the selection in image space
	pattern fPatternUp, fPatternDown, fPatternLeft, fPatternRight;
	
	bool fSlideShow;
	int fSlideShowDelay;
	int fSlideShowCountDown;
	
	bool fShowCaption;
	BString fCaption;
};

#endif /* _ShowImageView_h */
