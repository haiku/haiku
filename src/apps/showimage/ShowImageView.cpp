/*****************************************************************************/
// ShowImageView
// Written by Fernando Francisco de Oliveira, Michael Wilber, Michael Pfeiffer
//
// ShowImageView.cpp
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

#include <stdio.h>
#include <Debug.h>
#include <Message.h>
#include <ScrollBar.h>
#include <StopWatch.h>
#include <Alert.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <File.h>
#include <Bitmap.h>
#include <TranslatorRoster.h>
#include <BitmapStream.h>
#include <Rect.h>
#include <SupportDefs.h>
#include <Directory.h>

#include "ShowImageConstants.h"
#include "ShowImageView.h"

#ifndef min
#define min(a,b) ((a)>(b)?(b):(a))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define BORDER_WIDTH 16
#define BORDER_HEIGHT 16
#define PEN_SIZE 1.0f
const rgb_color kborderColor = { 0, 0, 0, 255 };

// use patterns to simulate marching ants for selection
void 
ShowImageView::InitPatterns()
{
	uchar p;
	uchar p1 = 0x33;
	uchar p2 = 0xCC;
	for (int i = 0; i <= 7; i ++) {
		fPatternLeft.data[i] = p1;
		fPatternRight.data[i] = p2;
		if ((i / 2) % 2 == 0) {
			p = 255;
		} else {
			p = 0;
		}
		fPatternUp.data[i] = p;
		fPatternDown.data[i] = ~p;
	}
}

void
ShowImageView::RotatePatterns()
{
	int i;
	uchar p;
	bool set;
	
	// rotate up
	p = fPatternUp.data[0];
	for (i = 0; i <= 6; i ++) {
		fPatternUp.data[i] = fPatternUp.data[i+1];	
	}
	fPatternUp.data[7] = p;
	
	// rotate down
	p = fPatternDown.data[7];
	for (i = 7; i >= 1; i --) {
		fPatternDown.data[i] = fPatternDown.data[i-1];
	}
	fPatternDown.data[0] = p;
	
	// rotate to left
	p = fPatternLeft.data[0];
	set = (p & 0x80) != 0;
	p <<= 1;
	p &= 0xfe;
	if (set) p |= 1;
	memset(fPatternLeft.data, p, 8);
	
	// rotate to right
	p = fPatternRight.data[0];
	set = (p & 1) != 0;
	p >>= 1;
	if (set) p |= 0x80;
	memset(fPatternRight.data, p, 8);
}

void
ShowImageView::Pulse()
{
	if (fbHasSelection) {
		RotatePatterns();
		DrawSelectionBox(fSelectionRect);
	}
	if (fDiaShow) {
		fDiaShowCountDown --;
		if (fDiaShowCountDown <= 0) {
			fDiaShowCountDown = fDiaShowDelay;
			if (!NextFile()) {
				FirstFile();
			}
		}
	}
}

ShowImageView::ShowImageView(BRect rect, const char *name, uint32 resizingMode,
	uint32 flags)
	: BView(rect, name, resizingMode, flags)
{
	InitPatterns();
	
	fpBitmap = NULL;
	fDocumentIndex = 1;
	fDocumentCount = 1;
	fbHasSelection = false;
	fResizeToViewBounds = false;
	fDiaShow = false;
	SetDiaShowDelay(3); // 3 seconds
	
	SetViewColor(B_TRANSPARENT_COLOR);
	SetHighColor(kborderColor);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetPenSize(PEN_SIZE);
}

ShowImageView::~ShowImageView()
{
	delete fpBitmap;
	fpBitmap = NULL;
}

bool
ShowImageView::IsImage(const entry_ref *pref)
{
	BFile file(pref, B_READ_ONLY);
	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	BMessage ioExtension;

	BTranslatorRoster *proster = BTranslatorRoster::Default();
	if (!proster)
		return false;

	if (ioExtension.AddInt32("/documentIndex", fDocumentIndex) != B_OK)
		return false;

	if (proster->Identify(&file, &ioExtension, &info, 0, NULL,
		B_TRANSLATOR_BITMAP) != B_OK)
		return false;
		
	return true;
}

// send message to parent about new image
void ShowImageView::Notify(const char* status)
{
	BMessage msg(MSG_UPDATE_STATUS);
	if (status != NULL) {
		msg.AddString("status", status);
	}
	msg.AddRef("ref", &fCurrentRef);
	BMessenger msgr(Window());
	msgr.SendMessage(&msg);

	FixupScrollBars();
	Invalidate();
}

void
ShowImageView::SetImage(const entry_ref *pref)
{
	delete fpBitmap;
	fpBitmap = NULL;
	fbHasSelection = false;
	fMakesSelection = false;
	
	entry_ref ref;
	if (!pref)
		ref = fCurrentRef;
	else
		ref = *pref;

	BTranslatorRoster *proster = BTranslatorRoster::Default();
	if (!proster)
		return;
	BFile file(&ref, B_READ_ONLY);
	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	BMessage ioExtension;
	if (ref != fCurrentRef)
		// if new image, reset to first document
		fDocumentIndex = 1;
	if (ioExtension.AddInt32("/documentIndex", fDocumentIndex) != B_OK)
		return;
	if (proster->Identify(&file, &ioExtension, &info, 0, NULL,
		B_TRANSLATOR_BITMAP) != B_OK)
		return;
	
	// Translate image data and create a new ShowImage window
	BBitmapStream outstream;
	if (proster->Translate(&file, &info, &ioExtension, &outstream,
		B_TRANSLATOR_BITMAP) != B_OK)
		return;
	if (outstream.DetachBitmap(&fpBitmap) != B_OK)
		return;
	fCurrentRef = ref;
	
	// get the number of documents (pages) if it has been supplied
	int32 documentCount = 0;
	if (ioExtension.FindInt32("/documentCount", &documentCount) == B_OK &&
		documentCount > 0)
		fDocumentCount = documentCount;
	else
		fDocumentCount = 1;
		
	Notify(info.name);
}

void
ShowImageView::ResizeToViewBounds(bool resize)
{
	if (fResizeToViewBounds != resize) {
		fResizeToViewBounds = resize;
		FixupScrollBars();
		Invalidate();
	}
}

BBitmap *
ShowImageView::GetBitmap()
{
	return fpBitmap;
}

void
ShowImageView::AttachedToWindow()
{
	FixupScrollBars();
}

BRect
ShowImageView::AlignBitmap() const
{
	BRect rect(fpBitmap->Bounds());
	if (fResizeToViewBounds) {
		float width, height;
		width = Bounds().Width()-2*PEN_SIZE;
		height = Bounds().Height()-2*PEN_SIZE;
		if (width > 0 && height > 0) {
			float s;
			s = width / rect.Width();
			
			if (s * rect.Height() <= height) {
				rect.right = width;
				rect.bottom = s * rect.Height();
				// center horizontally
				rect.OffsetBy(0, (height - rect.Height()) / 2);
			} else {
				rect.right = height / rect.Height() * rect.Width();
				rect.bottom = height;
				// center vertically
				rect.OffsetBy((width - rect.Width()) / 2, 0);
			}
		}
	} else {
		rect.OffsetBy(BORDER_WIDTH, BORDER_WIDTH);
	}
	rect.OffsetBy(PEN_SIZE, PEN_SIZE);
	return rect;
}

void
ShowImageView::Setup(BRect rect)
{
	fLeft = rect.left;
	fTop = rect.top;
	fScaleX = rect.Width() / fpBitmap->Bounds().Width();
	fScaleY = rect.Height() / fpBitmap->Bounds().Height();
// XXX Is Width/Height off by one?
//	fScaleX = (rect.Width()+1) / (fpBitmap->Bounds().Width()+1);
//	fScaleY = (rect.Width()+1) / (fpBitmap->Bounds().Height()+1);
}

BPoint
ShowImageView::ImageToView(BPoint p) const
{
	p.x = fScaleX * p.x + fLeft;
	p.y = fScaleY * p.y + fTop;
	return p;
}

BPoint
ShowImageView::ViewToImage(BPoint p) const
{
	p.x = (p.x - fLeft) / fScaleX;
	p.y = (p.y - fTop) / fScaleY;
	return p;
}

BRect
ShowImageView::ImageToView(BRect r) const
{
	BPoint leftTop(ImageToView(BPoint(r.left, r.top)));
	BPoint rightBottom(ImageToView(BPoint(r.right, r.bottom)));
	return BRect(leftTop.x, leftTop.y, rightBottom.x, rightBottom.y);
}

void
ShowImageView::DrawBorder(BRect border)
{
	BRect bounds(Bounds());
	// top
	FillRect(BRect(0, 0, bounds.right, border.top-1), B_SOLID_LOW);
	// left
	FillRect(BRect(0, border.top, border.left-1, border.bottom), B_SOLID_LOW);
	// right
	FillRect(BRect(border.right+1, border.top, bounds.right, border.bottom), B_SOLID_LOW);
	// bottom
	FillRect(BRect(0, border.bottom+1, bounds.right, bounds.bottom), B_SOLID_LOW);
}

void
ShowImageView::Draw(BRect updateRect)
{
	if (fpBitmap) {
		BRect rect = AlignBitmap();
		Setup(rect);
		
		BRect border(rect);
		border.InsetBy(-PEN_SIZE, -PEN_SIZE);

		DrawBorder(border);

		// Draw black rectangle around image
		StrokeRect(border);
		
		// Draw image	
		DrawBitmap(fpBitmap, fpBitmap->Bounds(), rect);
					
		if (fbHasSelection)
			DrawSelectionBox(fSelectionRect);
	}
}

void
ShowImageView::DrawSelectionBox(BRect &rect)
{
	PushState();
	rgb_color white = {255, 255, 255};
	SetLowColor(white);
	BRect r(ImageToView(rect));
	StrokeLine(BPoint(r.left, r.top), BPoint(r.right, r.top), fPatternLeft);
	StrokeLine(BPoint(r.right, r.top+1), BPoint(r.right, r.bottom-1), fPatternUp);
	StrokeLine(BPoint(r.left, r.bottom), BPoint(r.right, r.bottom), fPatternRight);
	StrokeLine(BPoint(r.left, r.top+1), BPoint(r.left, r.bottom-1), fPatternDown);
	PopState();
}

void
ShowImageView::FrameResized(float /* width */, float /* height */)
{
	FixupScrollBars();
}

void
ShowImageView::ConstrainToImage(BPoint &point)
{
	point.ConstrainTo(
		fpBitmap->Bounds()
	);
}

void
ShowImageView::MouseDown(BPoint point)
{
	point = ViewToImage(point);
	
	if (fbHasSelection && fSelectionRect.Contains(point)) {
		// TODO: start drag and drop
	} else {
		// begin new selection
		fMakesSelection = true;
		fbHasSelection = true;
		SetMouseEventMask(B_POINTER_EVENTS);
		ConstrainToImage(point);
		fFirstPoint = point;
		fSelectionRect.Set(point.x, point.y, point.x, point.y);
		Invalidate(); 
	}
}

void
ShowImageView::UpdateSelectionRect(BPoint point, bool final) {
	BRect oldSelection = fSelectionRect;
	point = ViewToImage(point);
	ConstrainToImage(point);
	fSelectionRect.left = min(fFirstPoint.x, point.x);
	fSelectionRect.right = max(fFirstPoint.x, point.x);
	fSelectionRect.top = min(fFirstPoint.y, point.y);
	fSelectionRect.bottom = max(fFirstPoint.y, point.y);
	if (final) {
		// selection must contain a few pixels
		if (fSelectionRect.Width() * fSelectionRect.Height() <= 1) {
			fbHasSelection = false;
		}
	}
	if (oldSelection != fSelectionRect || !fbHasSelection) {
		BRect updateRect;
		updateRect = oldSelection | fSelectionRect;
		updateRect = ImageToView(updateRect);
		updateRect.InsetBy(-PEN_SIZE, -PEN_SIZE);
		Invalidate(updateRect);
	}
}

void
ShowImageView::MouseMoved(BPoint point, uint32 state, const BMessage *pmsg)
{
	if (fMakesSelection) {
		UpdateSelectionRect(point, false);
	}
}

void
ShowImageView::MouseUp(BPoint point)
{
	if (fMakesSelection) {
		UpdateSelectionRect(point, true);
		fMakesSelection = false;
	}
}

void
ShowImageView::MessageReceived(BMessage *pmsg)
{
	switch (pmsg->what) {
		default:
			BView::MessageReceived(pmsg);
			break;
	}
}

void
ShowImageView::FixupScrollBars()
{
	BScrollBar *psb;	
	if (fResizeToViewBounds) {
		psb = ScrollBar(B_HORIZONTAL);
		if (psb) psb->SetRange(0, 0);
		psb = ScrollBar(B_VERTICAL);
		if (psb) psb->SetRange(0, 0);
		return;
	}

	BRect rctview = Bounds(), rctbitmap(0, 0, 0, 0);
	if (fpBitmap)
		rctbitmap = fpBitmap->Bounds();
	
	float prop, range;
	psb = ScrollBar(B_HORIZONTAL);
	if (psb) {
		range = rctbitmap.Width() + (BORDER_WIDTH * 2) - rctview.Width();
		if (range < 0) range = 0;
		prop = rctview.Width() / (rctbitmap.Width() + (BORDER_WIDTH * 2));
		if (prop > 1.0f) prop = 1.0f;
		psb->SetRange(0, range);
		psb->SetProportion(prop);
		psb->SetSteps(10, 100);
	}
	
	psb = ScrollBar(B_VERTICAL);
	if (psb) {
		range = rctbitmap.Height() + (BORDER_HEIGHT * 2) - rctview.Height();
		if (range < 0) range = 0;
		prop = rctview.Height() / (rctbitmap.Height() + (BORDER_HEIGHT * 2));
		if (prop > 1.0f) prop = 1.0f;
		psb->SetRange(0, range);
		psb->SetProportion(prop);
		psb->SetSteps(10, 100);
	}
}

int32
ShowImageView::CurrentPage()
{
	return fDocumentIndex;
}

int32
ShowImageView::PageCount()
{
	return fDocumentCount;
}

void
ShowImageView::FirstPage()
{
	if (fDocumentIndex != 1) {
		fDocumentIndex = 1;
		SetImage(NULL);
	}
}

void
ShowImageView::LastPage()
{
	if (fDocumentIndex != fDocumentCount) {
		fDocumentIndex = fDocumentCount;
		SetImage(NULL);
	}
}

void
ShowImageView::NextPage()
{
	if (fDocumentIndex < fDocumentCount) {
		fDocumentIndex++;
		SetImage(NULL);
	}
}

void
ShowImageView::PrevPage()
{
	if (fDocumentIndex > 1) {
		fDocumentIndex--;
		SetImage(NULL);
	}
}

void
ShowImageView::FreeEntries(BList* entries)
{
	const int32 n = entries->CountItems();
	for (int32 i = 0; i < n; i ++) {
		BEntry* entry = (BEntry*)entries->ItemAt(i);
		delete entry;		
	}
	entries->MakeEmpty();
}

// Notes: BDirectory::GetNextEntry() must not necessary
// return entries sorted by name, thou BFS does. 
bool
ShowImageView::FindNextImage(BEntry* image, bool next, bool rewind)
{
	ASSERT(next || !rewind);
	BEntry curImage(&fCurrentRef);
	BEntry entry;
	BDirectory parent;
	BList entries;

	if (curImage.GetParent(&parent) != B_OK)
		return false;

	while (parent.GetNextEntry(&entry) == B_OK) {
		if (rewind || entry == curImage) {
			// start with first entry or found current image
			if (next) {
				// start with next entry
				if (!rewind && parent.GetNextEntry(&entry) != B_OK) {
					break;
				}
				// find next image in directory
				do {
					entry_ref ref;
					if (entry.GetRef(&ref) == B_OK && IsImage(&ref)) {
						*image = entry;
						return true;
					}
				} while (parent.GetNextEntry(&entry) == B_OK);
			} else {
				// find previous image in directory
				for (int32 i = entries.CountItems() - 1; i >= 0; i --) {
					BEntry* entry = (BEntry*)entries.ItemAt(i);
					entry_ref ref;
					if (entry->GetRef(&ref) == B_OK && IsImage(&ref)) {
						*image = *entry;
						FreeEntries(&entries);
						return true;
					}
				}
			}
			break;
		}
		if (!next) {			
			// record entries if we search for previous file
			entries.AddItem(new BEntry(entry));
		}
	}
	FreeEntries(&entries);
	return false;
}

bool
ShowImageView::ShowNextImage(bool next, bool rewind)
{
	BEntry image;
	entry_ref ref;
	
	if (FindNextImage(&image, next, rewind) && image.GetRef(&ref) == B_OK) {
		SetImage(&ref); 
		return true;
	}
	return false;
}

bool
ShowImageView::NextFile()
{
	return ShowNextImage(true, false);
}

bool
ShowImageView::PrevFile()
{
	return ShowNextImage(false, false);
}

bool
ShowImageView::FirstFile()
{
	return ShowNextImage(true, true);
}

void
ShowImageView::SetDiaShowDelay(float seconds)
{
	fDiaShowDelay = (int)(seconds * 10.0);
}

void
ShowImageView::StartDiaShow()
{
	fDiaShow = true; fDiaShowCountDown = fDiaShowDelay;
}

void
ShowImageView::StopDiaShow()
{
	fDiaShow = false;
}

int32 
ShowImageView::BytesPerPixel(color_space cs) const
{
	switch (cs) {
		case B_RGB32:      // fall through
		case B_RGB32_BIG:  // fall through
		case B_RGBA32:     // fall through
		case B_RGBA32_BIG: return 4;

		case B_RGB24_BIG:  // fall through
		case B_RGB24:      return 3;

		case B_RGB16:      // fall through
		case B_RGB16_BIG:  // fall through
		case B_RGB15:      // fall through
		case B_RGB15_BIG:  // fall through
		case B_RGBA15:     // fall through
		case B_RGBA15_BIG: return 2;

		case B_GRAY8:      // fall through
		case B_CMAP8:      return 1;
		case B_GRAY1:      return 0;
		default: return -1;
	}
}

void
ShowImageView::CopyPixel(uchar* dest, int32 destX, int32 destY, int32 destBPR, uchar* src, int32 x, int32 y, int32 bpr, int32 bpp)
{
	dest += destBPR * destY + destX * bpp;
	src += bpr * y + x * bpp;
	memcpy(dest, src, bpp);
}

// FIXME: In color space with alpha channel the alpha value should not be inverted!
// This could be a problem when image is saved and opened in another application.
// Note: For B_CMAP8 InvertPixel inverts the color index not the color value!
void
ShowImageView::InvertPixel(int32 x, int32 y, uchar* dest, int32 destBPR, uchar* src, int32 bpr, int32 bpp)
{
	dest += destBPR * y + x * bpp;
	src += bpr * y + x * bpp;
	for (; bpp > 0; bpp --, dest ++, src ++) {
		*dest = ~*src;
	}	
}

// DoImageOperation supports only color spaces with bytes per pixel >= 1
// See above for limitations about kInvert
void
ShowImageView::DoImageOperation(image_operation op) 
{
	color_space cs;
	int32 bpp;
	BBitmap* bm;
	int32 width, height;
	uchar* src;
	uchar* dest;
	int32 bpr, destBPR;
	int32 x, y, destX, destY;
	BRect rect;
	
	if (fpBitmap == NULL) return;
	
	cs = fpBitmap->ColorSpace();
	bpp = BytesPerPixel(cs);	
	if (bpp < 1) return;
	
	width = fpBitmap->Bounds().IntegerWidth();
	height = fpBitmap->Bounds().IntegerHeight();
	
	if (op == kRotateClockwise || op == kRotateAntiClockwise) {
		rect.Set(0, 0, height, width);
	} else {
		rect.Set(0, 0, width, height);
	}
	
	bm = new BBitmap(rect, cs);
	if (bm == NULL) return;
	
	src = (uchar*)fpBitmap->Bits();
	dest = (uchar*)bm->Bits();
	bpr = fpBitmap->BytesPerRow();
	destBPR = bm->BytesPerRow();
	
	switch (op) {
		case kRotateClockwise:
			for (y = 0; y <= height; y ++) {
				for (x = 0; x <= width; x ++) {
					destX = height - y;
					destY = x;
					CopyPixel(dest, destX, destY, destBPR, src, x, y, bpr, bpp);
				}
			}
			break;
		case kRotateAntiClockwise:
			for (y = 0; y <= height; y ++) {
				for (x = 0; x <= width; x ++) {
					destX = y;
					destY = width - x;
					CopyPixel(dest, destX, destY, destBPR, src, x, y, bpr, bpp);
				}
			}
			break;
		case kMirrorHorizontal:
			for (y = 0; y <= height; y ++) {
				for (x = 0; x <= width; x ++) {
					destX = x;
					destY = height - y;
					CopyPixel(dest, destX, destY, destBPR, src, x, y, bpr, bpp);
				}
			}
			break;
		case kMirrorVertical:
			for (y = 0; y <= height; y ++) {
				for (x = 0; x <= width; x ++) {
					destX = width - x;
					destY = y;
					CopyPixel(dest, destX, destY, destBPR, src, x, y, bpr, bpp);
				}
			}
			break;
		case kInvert:
			for (y = 0; y <= height; y ++) {
				for (x = 0; x <= width; x ++) {
					InvertPixel(x, y, dest, destBPR, src, bpr, bpp);
				}
			}
			break;
	}

	// set new bitmap
	delete fpBitmap; fpBitmap = bm; 
	// remove selection
	fbHasSelection = false;
	Notify(NULL);	
}

void
ShowImageView::Rotate(int degree)
{
	if (degree == 90) {
		DoImageOperation(kRotateClockwise);
	} else if (degree == 270) {
		DoImageOperation(kRotateAntiClockwise);
	}
}

void
ShowImageView::Mirror(bool vertical) 
{
	if (vertical) {
		DoImageOperation(kMirrorVertical);
	} else {
		DoImageOperation(kMirrorHorizontal);
	}
}

void
ShowImageView::Invert()
{
	DoImageOperation(kInvert);
}
