/*

Preview

Copyright (c) 2002, 2003 OpenBeOS. 
Copyright (c) 2005 Haiku.

Author: 
	Michael Pfeiffer
	Hartmut Reh
	
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

#include <InterfaceKit.h>
#include "PrintJobReader.h"
#include "InterfaceUtils.h"

class PreviewPage {
	int32 fPage;
	int32 fNumberOfPictures;
	BPicture* fPictures;
	BPoint* fPoints;
	BRect* fRects;
	status_t fStatus;
	
public:
	PreviewPage(int32 page, PrintJobPage* pjp);
	~PreviewPage();
	status_t InitCheck() const;
	
	int32 Page() const { return fPage; }
	void Draw(BView* view);
};

class PreviewView : public BView {
	int32 fPage;
	int32 fZoom;
	PrintJobReader fReader;
	int32 fNumberOfPagesPerPage;
	int32 fNumberOfPages;        // physical pages
	bool fReverse;
	BRect fPageRect;
	JobData::Orientation fOrientation;
	JobData::PageSelection fPageSelection;
	PreviewPage* fCachedPage;
	
	float ZoomFactor() const;
	BRect PageRect() const;
	BRect PrintableRect() const;
	int32 GetPageNumber(int32 index) const;
	
public:
	PreviewView(BFile* jobFile, BRect rect);
	~PreviewView();
	status_t InitCheck() const;
	
	BRect ViewRect() const;

	bool IsPageLoaded(int32 page) const;
	bool IsPageValid() const;
	void LoadPage(int32 page);
	void DrawPageFrame(BRect r);
	void DrawPage(BRect r);
	void Draw(BRect r);
	void FrameResized(float width, float height);
	
	void FixScrollbars();
	
	bool ShowsFirstPage() const;
	bool ShowsLastPage() const;
	int CurrentPage() const { return fPage + 1; }
	int NumberOfPages() const;
	void ShowNextPage();
	void ShowPrevPage();
	void ShowFirstPage();
	void ShowLastPage();
	void ShowFindPage(int page);
	
	bool CanZoomIn() const;
	bool CanZoomOut() const;
	void ZoomIn();
	void ZoomOut();
};

class PreviewWindow : public BlockingWindow {
	BButton *fFirst;
	BButton *fNext;
	BButton *fPrev;
	BButton *fLast;
	BButton *fZoomIn;
	BButton *fZoomOut;
	BTextControl *fPageNumber;
	BStringView *fPageText;
	PreviewView *fPreview;
	BScrollView *fPreviewScroller;
	float fButtonBarHeight;
	
	enum {
		MSG_FIRST_PAGE = 'pwfp',			
		MSG_NEXT_PAGE  = 'pwnp',
		MSG_PREV_PAGE  = 'pwpp',
		MSG_LAST_PAGE  = 'pwlp',			
		MSG_FIND_PAGE  = 'pwsp',
		MSG_ZOOM_IN    = 'pwzi',
		MSG_ZOOM_OUT   = 'pwzo',
		MSG_PRINT_JOB  = 'pwpj',
		MSG_CANCEL_JOB = 'pwcj',
	};

	void ResizeToPage(bool allowShrinking = false);
	void UpdateControls();
	
	typedef BlockingWindow inherited; 
	
public:
	PreviewWindow(BFile* jobFile, bool showOkAndCancelButtons = false);
	status_t InitCheck() const { return fPreview->InitCheck(); }
	void MessageReceived(BMessage* m);
	status_t Go();
};

