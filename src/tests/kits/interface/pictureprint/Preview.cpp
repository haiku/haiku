/*

Preview

Copyright (c) 2002 Haiku.

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
		view->SetHighColor(255, 0, 0);
		view->StrokeRect(fRects[i]);
	}
}

// Implementation of PreviewView

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

status_t PreviewView::InitCheck() const {
	return fReader.InitCheck();
}

void PreviewView::Draw(BRect rect) {
	if (fReader.InitCheck() == B_OK) {
			// set zoom factor
		SetScale(ZoomFactor());
			// render picture(s) of current page
		if (fCachedPage == NULL || fCachedPage->Page() != fPage) {
			delete fCachedPage; fCachedPage = NULL;
			PrintJobPage page;
			if (fReader.GetPage(fPage, page) == B_OK) 
				fCachedPage = new PreviewPage(fPage, &page);
		}
		if (fCachedPage && fCachedPage->InitCheck() == B_OK) 
			fCachedPage->Draw(this);
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
	float width = PageRect().Width();
	float height = PageRect().Height();
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
	: BWindow(BRect(20, 20, 400, 600), "Preview", B_DOCUMENT_WINDOW, 0)
{
	float top = 0;
	float left = 10;
	
	BRect r = Frame();
	r.right = r.IntegerWidth() - B_V_SCROLL_BAR_WIDTH; r.left = 0;
	r.bottom = r.IntegerHeight() - B_H_SCROLL_BAR_HEIGHT; r.top = 0;

		// add navigation and zoom buttons
	fNext = new BButton(BRect(left, top, left+10, top+10), "Next", "Next Page", new BMessage(MSG_NEXT_PAGE));
	AddChild(fNext);
	fNext->ResizeToPreferred();
	left = fNext->Frame().right + 10;
	
	fPrev = new BButton(BRect(left, top, left+10, top+10), "Prev", "Previous Page", new BMessage(MSG_PREV_PAGE));
	AddChild(fPrev);
	fPrev->ResizeToPreferred();
	left = fPrev->Frame().right + 10;
	
	fZoomIn = new BButton(BRect(left, top, left+10, top+10), "ZoomIn", "Zoom In", new BMessage(MSG_ZOOM_IN));
	AddChild(fZoomIn);
	fZoomIn->ResizeToPreferred();
	left = fZoomIn->Frame().right + 10;
	
	fZoomOut = new BButton(BRect(left, top, left+10, top+10), "ZoomOut", "Zoom Out", new BMessage(MSG_ZOOM_OUT));
	AddChild(fZoomOut);
	fZoomOut->ResizeToPreferred();
	left = fZoomOut->Frame().right + 10;
	
	top = fZoomOut->Frame().bottom + 5;
	
		// add preview view		
	r.top = top;
	fPreview = new PreviewView(jobFile, r);
	fPreviewScroller = new BScrollView("PreviewScroller", fPreview, B_FOLLOW_ALL, 0, true, true, B_FANCY_BORDER);
	fPreviewScroller->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	AddChild(fPreviewScroller);
	
	if (fPreview->InitCheck() == B_OK) {
		fPreview->FixScrollbars();
		UpdateControls();
		Show();
	} else {
		Quit();
	}
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
		case MSG_ZOOM_IN: fPreview->ZoomIn();
			break;
		case MSG_ZOOM_OUT: fPreview->ZoomOut();
			break;
		default:
			inherited::MessageReceived(m); return;
	}
	UpdateControls();
}

#if 1

// Standalone test preview application
// Note to quit application press Command+Q

#include <Application.h>

class PreviewApp : public BApplication {
public:
	PreviewApp(const char* sig) : BApplication(sig) { }
	void ArgvReceived(int32 argc, char* argv[]);
};

void PreviewApp::ArgvReceived(int32 argc, char* argv[]) {
	for (int32 i = 1; i < argc; i ++) {
		BFile jobFile(argv[i], B_READ_WRITE);
		if (jobFile.InitCheck() == B_OK) {
			new PreviewWindow(&jobFile);
		}
	}
}

int main(int argc, char* argv[]) {
	PreviewApp app("application/x-vnd.obos.preview");
	app.Run();	
}
#endif
