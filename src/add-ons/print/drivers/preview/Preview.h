/*
 * Copyright 2002-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 *		Hartmut Reh
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
	PreviewPage* fCachedPage;
	
	float ZoomFactor() const;
	BRect PageRect() const;
	BRect PrintableRect() const;
	
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
	};

	void ResizeToPage();
	void UpdateControls();
	
	typedef BlockingWindow inherited; 
	
public:
	PreviewWindow(BFile* jobFile);
	status_t InitCheck() const { return fPreview->InitCheck(); }
	void MessageReceived(BMessage* m);
	status_t Go();
};

