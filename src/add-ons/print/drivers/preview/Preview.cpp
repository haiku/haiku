/*

Preview

Copyright (c) 2002, 2003 OpenBeOS. 

Author: 
	Michael Pfeiffer
	
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <Debug.h>

#include "Preview.h"

// Implementation of PreviewPage
PreviewPage::PreviewPage(int32 page, PrintJobPage* pjp)
	: fPage(page)
	, fPictures(NULL)
	, fPoints(NULL)
	, fRects(NULL)
{
	fNumberOfPictures = pjp->NumberOfPictures();
	fPictures = new BPicture[fNumberOfPictures];
	fPoints = new BPoint[fNumberOfPictures];
	fRects = new BRect[fNumberOfPictures];
	status_t rc = B_ERROR;
	for (int32 i = 0; i < fNumberOfPictures && 
		(rc = pjp->NextPicture(fPictures[i], fPoints[i], fRects[i])) == B_OK; i ++);
	fStatus = rc;
}

PreviewPage::~PreviewPage() {
	delete []fPictures;
	delete []fPoints;
	delete []fRects;
}

status_t PreviewPage::InitCheck() const {
	return fStatus;
}

void PreviewPage::Draw(BView* view) {
	ASSERT(fStatus == B_OK);
	for (int32 i = 0; i < fNumberOfPictures; i ++) {
		view->DrawPicture(&fPictures[i], fPoints[i]);
	}
}

// Implementation of PreviewView

const float kPreviewTopMargin = 10;
const float kPreviewBottomMargin = 30;
const float kPreviewLeftMargin = 10;
const float kPreviewRightMargin = 30;

PreviewView::PreviewView(BFile* jobFile, BRect rect)
	: BView(rect, "PreviewView", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
	, fPage(0)
	, fZoom(0)
	, fReader(jobFile)
	, fCachedPage(NULL)
{
}

PreviewView::~PreviewView() {
	delete fCachedPage;
}

// returns 2 ^ fZoom
float PreviewView::ZoomFactor() const {
	const int32 b = 4;
	int32 zoom;
	if (fZoom > 0) zoom = (1 << b) << fZoom;
	else zoom = (1 << b) >> -fZoom;
	return zoom / (float)(1 << b);
}

BRect PreviewView::PageRect() const {
	float f = ZoomFactor();
	BRect r = fReader.PaperRect();
	r.left *= f; r.right *= f; r.top *= f; r.bottom *= f;
	return r;
}

BRect PreviewView::ViewRect() const {
	BRect r(PageRect());
	r.right += kPreviewLeftMargin + kPreviewRightMargin;
	r.bottom += kPreviewTopMargin + kPreviewBottomMargin;
	return r;
}

status_t PreviewView::InitCheck() const {
	return fReader.InitCheck();
}

bool PreviewView::IsPageLoaded(int32 page) const {
	return fCachedPage != NULL && fCachedPage->Page() == page;
}

bool PreviewView::IsPageValid() const {
	return fCachedPage && fCachedPage->InitCheck() == B_OK;
}

void PreviewView::LoadPage(int32 page) {
	delete fCachedPage; fCachedPage = NULL;
	PrintJobPage pjp;
	if (fReader.GetPage(page, pjp) == B_OK) {
		fCachedPage = new PreviewPage(page, &pjp);
	}
}

void PreviewView::DrawPageFrame(BRect rect) {
	const float kShadowIndent = 3;
	const float kShadowWidth = 3;
	float x, y;
	float right, bottom;
	rgb_color frameColor = {0, 0, 0, 0};
	rgb_color shadowColor = {90, 90, 90, 0};
	BRect r(PageRect());
	
	PushState();
	// draw page border around page
	r.InsetBy(-1, -1);
	r.OffsetTo(kPreviewLeftMargin-1, kPreviewTopMargin-1);
	SetHighColor(frameColor);
	StrokeRect(r);
	
	// draw page shadow
	SetHighColor(shadowColor);

	x = r.right + 1;
	right = x + kShadowWidth;
	bottom = r.bottom + 1 + kShadowWidth;
	y = r.top + kShadowIndent;
	FillRect(BRect(x, y, right, bottom));

	x = r.left + kShadowIndent;
	y = r.bottom  + 1;
	FillRect(BRect(x, y, r.right, bottom));
	PopState();
}

void PreviewView::DrawPage(BRect rect) {
	// constrain clipping region to paper dimensions
	BRect r(PageRect());
	r.OffsetBy(kPreviewLeftMargin, kPreviewTopMargin);
	BRegion clip(r);
	ConstrainClippingRegion(&clip);

	// draw page contents
	PushState();
	SetOrigin(kPreviewLeftMargin, kPreviewTopMargin);
	SetScale(ZoomFactor());
	fCachedPage->Draw(this);
	PopState();
}

void PreviewView::Draw(BRect rect) {
	if (fReader.InitCheck() == B_OK) {
		if (!IsPageLoaded(fPage)) {
			LoadPage(fPage);
		}
		if (IsPageValid()) {
			DrawPageFrame(rect);
			DrawPage(rect);
		}
	}
}

void PreviewView::FrameResized(float width, float height) {
	FixScrollbars();
}

bool PreviewView::ShowsFirstPage() const {
	return fPage == 0;
}

bool PreviewView::ShowsLastPage() const {
	return fPage == NumberOfPages() - 1;
}

int PreviewView::NumberOfPages() const {
	return fReader.NumberOfPages();
}

void PreviewView::ShowNextPage() {
	if (!ShowsLastPage()) {
		fPage ++; Invalidate();
	}
}

void PreviewView::ShowPrevPage() {
	if (!ShowsFirstPage()) {
		fPage --; Invalidate();
	}
}
	
bool PreviewView::CanZoomIn() const {
	return fZoom < 4;
}

bool PreviewView::CanZoomOut() const {
	return fZoom > -2;
}

void PreviewView::ZoomIn() {
	if (CanZoomIn()) {
		fZoom ++; FixScrollbars(); Invalidate();
	}
}

void PreviewView::ZoomOut() {
	if (CanZoomOut()) {
		fZoom --; FixScrollbars(); Invalidate();
	}
}

void PreviewView::FixScrollbars() {
	BRect frame = Bounds();
	BScrollBar * scroll;
	float x, y;
	float bigStep, smallStep;
	float width = PageRect().Width() + kPreviewLeftMargin + kPreviewRightMargin;
	float height = PageRect().Height() + kPreviewTopMargin + kPreviewBottomMargin;
	x = width - frame.Width();
	if (x < 0.0) {
		x = 0.0;
	}
	y = height - frame.Height();
	if (y < 0.0) {
		y = 0.0;
	}
	
	scroll = ScrollBar (B_HORIZONTAL);
	scroll->SetRange (0.0, x);
	scroll->SetProportion ((width - x) / width);
	bigStep = frame.Width() - 2;
	smallStep = bigStep / 10.;
	scroll->SetSteps (smallStep, bigStep);

	scroll = ScrollBar (B_VERTICAL);
	scroll->SetRange (0.0, y);
	scroll->SetProportion ((height - y) / height);
	bigStep = frame.Height() - 2;
	smallStep = bigStep / 10.;
	scroll->SetSteps (smallStep, bigStep);
}

// Implementation of PreviewWindow

PreviewWindow::PreviewWindow(BFile* jobFile) 
	: BlockingWindow(BRect(20, 24, 400, 600), "Preview", B_DOCUMENT_WINDOW, 0)
{
	float top = 7;
	float left = 20;
	float width, height;
		
	BRect r = Frame();
	r.right = r.IntegerWidth() - B_V_SCROLL_BAR_WIDTH; r.left = 0;
	r.bottom = r.IntegerHeight() - B_H_SCROLL_BAR_HEIGHT; r.top = 0;

		// add navigation and zoom buttons
	fPrev = new BButton(BRect(left, top, left+10, top+10), "Prev", "Previous Page", new BMessage(MSG_PREV_PAGE));
	AddChild(fPrev);
	fPrev->ResizeToPreferred();
	width = fPrev->Bounds().Width()+1;
	height = fPrev->Bounds().Height()+1;
	left = fPrev->Frame().right + 30;
	
	fNext = new BButton(BRect(left, top, left+10, top+10), "Next", "Next Page", new BMessage(MSG_NEXT_PAGE));
	AddChild(fNext);
	fNext->ResizeTo(width, height);
	left = fNext->Frame().right + 70;
	
	fZoomIn = new BButton(BRect(left, top, left+10, top+10), "ZoomIn", "Zoom In", new BMessage(MSG_ZOOM_IN));
	AddChild(fZoomIn);
	fZoomIn->ResizeTo(width, height);
	left = fZoomIn->Frame().right + 30;
	
	fZoomOut = new BButton(BRect(left, top, left+10, top+10), "ZoomOut", "Zoom Out", new BMessage(MSG_ZOOM_OUT));
	AddChild(fZoomOut);
	fZoomOut->ResizeTo(width, height);
	
	fButtonBarHeight = fZoomOut->Frame().bottom + 7;
	
		// add preview view		
	r.top = fButtonBarHeight;
	fPreview = new PreviewView(jobFile, r);
	fPreviewScroller = new BScrollView("PreviewScroller", fPreview, B_FOLLOW_ALL, 0, true, true, B_FANCY_BORDER);
	fPreviewScroller->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fPreviewScroller);
	
	if (fPreview->InitCheck() == B_OK) {
		ResizeToPage();
		fPreview->FixScrollbars();
		UpdateControls();
	}
}

void PreviewWindow::ResizeToPage() {
	BScreen screen;
	BRect r(fPreview->ViewRect());
	float width, height;
	float maxWidth, maxHeight, minWidth;
	const float windowBorderWidth = 5;
	const float windowBorderHeight = 5;
	
	if (screen.Frame().right == 0.0) {	
		return; // invalid screen object
	}
	
	width = r.Width() + 1 + B_V_SCROLL_BAR_WIDTH;
	height = r.Height() + 1 + fButtonBarHeight + B_H_SCROLL_BAR_HEIGHT;

	// dimensions so that window does not reach outside of screen	
	maxWidth = screen.Frame().Width() + 1 - windowBorderWidth - Frame().left;
	maxHeight = screen.Frame().Height() + 1 - windowBorderHeight - Frame().top;

	// width so that all buttons are visible
	minWidth = 	fZoomOut->Frame().right + 10;
	
	if (width < minWidth) width = minWidth;

	if (width > maxWidth) width = maxWidth;
	if (height > maxHeight) height = maxHeight;
	
	ResizeTo(width, height);
}

void PreviewWindow::UpdateControls() {
	fPrev->SetEnabled(!fPreview->ShowsFirstPage());
	fNext->SetEnabled(!fPreview->ShowsLastPage());
	fZoomIn->SetEnabled(fPreview->CanZoomIn());
	fZoomOut->SetEnabled(fPreview->CanZoomOut());
}

void PreviewWindow::MessageReceived(BMessage* m) {
	switch (m->what) {
		case MSG_NEXT_PAGE: fPreview->ShowNextPage();
			break;
		case MSG_PREV_PAGE: fPreview->ShowPrevPage();
			break;
		case MSG_ZOOM_IN: fPreview->ZoomIn(); ResizeToPage();
			break;
		case MSG_ZOOM_OUT: fPreview->ZoomOut(); ResizeToPage();
			break;
		default:
			inherited::MessageReceived(m); return;
	}
	UpdateControls();
}

status_t PreviewDriver::PrintJob(BFile *jobFile, BMessage *jobMsg) {
	PreviewWindow* w;
	status_t st;
	w = new PreviewWindow(jobFile);
	st = w->InitCheck();
	if (st == B_OK) {
		w->Go();
	} else {
		w->Quit();
	}
	return st;
}

PrinterDriver* instanciate_driver(BNode *spoolDir)
{
	return new PreviewDriver(spoolDir);
}

// About dialog text:
const char* 
kAbout =
"Preview for BeOS\n"
"© 2003 OpenBeOS\n"
"by Michael Pfeiffer\n"
"\n"
"Based on PDF Writer by\nPhilippe Houdoin, Simon Gauvin, Michael Pfeiffer\n"
;


