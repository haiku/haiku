/*
 * Copyright 2002-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 *		Hartmut Reh
 */

#include <stdlib.h>

#include <Debug.h>
#include <String.h>
#include "BeUtils.h"
#include "Utils.h"

#include "GraphicsDriver.h"
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

void PreviewPage::Draw(BView* view) 
{
	ASSERT(fStatus == B_OK);
	for (int32 i = 0; i < fNumberOfPictures; i ++) 
	{
		view->DrawPicture(&fPictures[i], fPoints[i]);
	}
}

// Implementation of PreviewView

const float kPreviewTopMargin = 10;
const float kPreviewBottomMargin = 30;
const float kPreviewLeftMargin = 10;
const float kPreviewRightMargin = 30;

// TODO share constant with JobData
static const char *kJDOrientation           = "orientation";
static const char *kJDNup                   = "JJJJ_nup";
static const char *kJDReverse               = "JJJJ_reverse";
static const char* kJDPageSelection         = "JJJJ_page_selection";

static BRect RotateRect(BRect rect)
{
	BRect rotated(rect.top, rect.left, rect.bottom, rect.right);
	return rotated;
}

PreviewView::PreviewView(BFile* jobFile, BRect rect)
	: BView(rect, "PreviewView", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
	, fPage(0)
	, fZoom(0)
	, fReader(jobFile)
	, fNumberOfPagesPerPage(1)
	, fReverse(false)
	, fOrientation(JobData::kPortrait)
	, fPageSelection(JobData::kAllPages)
	, fCachedPage(NULL)
{
	int32 value32;
	if (fReader.JobSettings()->FindInt32(kJDOrientation, &value32) == B_OK) {
		fOrientation = (JobData::Orientation)value32;
	}
	if (fReader.JobSettings()->FindInt32(kJDPageSelection, &value32) == B_OK) {
		fPageSelection = (JobData::PageSelection)value32;
	}
	fReader.JobSettings()->FindInt32(kJDNup, &fNumberOfPagesPerPage);
	fReader.JobSettings()->FindBool(kJDReverse, &fReverse);
	fNumberOfPages = (fReader.NumberOfPages() + fNumberOfPagesPerPage - 1) / fNumberOfPagesPerPage;
	if (fPageSelection == JobData::kOddNumberedPages) {
		fNumberOfPages = (fNumberOfPages + 1) / 2;
	}
	else if (fPageSelection == JobData::kEvenNumberedPages) {
		fNumberOfPages /= 2;		
	}
	fPageRect = fReader.PaperRect();
	switch (fNumberOfPagesPerPage) {
		case 2:
		case 8:
		case 32:
		case 128:
			fPageRect = RotateRect(fPageRect);
			break;
	}
}

PreviewView::~PreviewView() {
	delete fCachedPage;
}

// returns 2 ^ fZoom
float PreviewView::ZoomFactor() const {
	const int32 b = 4;
	int32 zoom;
	if (fZoom > 0) {
		zoom = (1 << b) << fZoom;
	} else {
		zoom = (1 << b) >> -fZoom;
	}
	float factor = zoom / (float)(1 << b);
	return factor * fReader.GetScale() / 100.0;
}

BRect PreviewView::PageRect() const {
	float f = ZoomFactor();
	BRect r(fPageRect);
	return ScaleRect(r, f);
}

BRect PreviewView::PrintableRect() const {
	float f = ZoomFactor();
	BRect r = fReader.PrintableRect();
	return ScaleRect(r, f);
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
	delete fCachedPage; 
	fCachedPage = NULL;
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


int32 PreviewView::GetPageNumber(int32 index) const
{
	int page = fPage;
	if (fReverse) {
		page = fNumberOfPages - fPage - 1;
	}

	if (fPageSelection == JobData::kOddNumberedPages) {
		page *= 2; // 0, 2, 4, ...
	}
	else if (fPageSelection == JobData::kEvenNumberedPages) {
		page = 2 * page + 1; // 1, 3, 5, ...
	}
	
	return page * fNumberOfPagesPerPage + index;
}

void PreviewView::DrawPage(BRect rect) 
{		

	BRect physicalRect = fReader.PaperRect();
	BRect scaledPhysicalRect = physicalRect;
	BRect scaledPrintableRect = fReader.PrintableRect();
	
	for (int32 index = 0; index < fNumberOfPagesPerPage; index ++) {
		int32 pageNumber = GetPageNumber(index);
		if (pageNumber < 0) {
			// no page at index
			continue;
		}
		if (!IsPageLoaded(pageNumber)) {
			LoadPage(pageNumber);
		}
		if (!IsPageValid()) {
			// TODO show feedback 
			continue;
		}
	
		BPoint scalingXY = GraphicsDriver::getScale(fNumberOfPagesPerPage,  physicalRect, 100.0);
		float scaling = min_c(scalingXY.x, scalingXY.y);
		BPoint offset = GraphicsDriver::getOffset(fNumberOfPagesPerPage, index, fOrientation, &scalingXY, 
			scaledPhysicalRect, scaledPrintableRect, physicalRect);
	
		// draw page contents
		PushState();
		
		// print job coordinates are relative to the printable rect
		SetScale(ZoomFactor() * scaling);
		
		BPoint leftTopMargin(BPoint(kPreviewLeftMargin, kPreviewTopMargin));
		leftTopMargin.x /= ZoomFactor() * scaling;
		leftTopMargin.y /= ZoomFactor() * scaling;
		offset += leftTopMargin;
		SetOrigin(offset);
		
		// constrain clipping region to printable rectangle
		BRect clipRect(fReader.PrintableRect());
		clipRect.OffsetTo(0, 0);
		BRegion clip(clipRect);
		ConstrainClippingRegion(&clip);
		
		fCachedPage->Draw(this);
		
		PopState();
	}
}

void PreviewView::Draw(BRect rect) {
	if (fReader.InitCheck() == B_OK) {
		DrawPageFrame(rect);
		DrawPage(rect);
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
	return fNumberOfPages;
}

void PreviewView::ShowNextPage() {
	if (!ShowsLastPage()) {
		fPage ++; 
		Invalidate();
	}
}

void PreviewView::ShowPrevPage() {
	if (!ShowsFirstPage()) {
		fPage --; 
		Invalidate();
	}
}
	
void PreviewView::ShowFirstPage() {
	if (!ShowsFirstPage()) {
		fPage = 0; 
		Invalidate();
	}
}

void PreviewView::ShowLastPage() {
	if (!ShowsLastPage()) {
		fPage = NumberOfPages()-1; 
		Invalidate();
	}
}

void PreviewView::ShowFindPage(int page) {
	page --;
	
	if (page < 0) {
		page = 0;
	} else if (page > (NumberOfPages()-1)) {
		page = NumberOfPages()-1;
	}
	
	fPage = (int32)page;

	Invalidate();
}

bool PreviewView::CanZoomIn() const {
	return fZoom < 4;
}

bool PreviewView::CanZoomOut() const {
	return fZoom > -2;
}

void PreviewView::ZoomIn() {
	if (CanZoomIn()) {
		fZoom ++; FixScrollbars(); 
		Invalidate();
	}
}

void PreviewView::ZoomOut() {
	if (CanZoomOut()) {
		fZoom --; FixScrollbars(); 
		Invalidate();
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

PreviewWindow::PreviewWindow(BFile* jobFile, bool showOkAndCancelButtons) 
	: BlockingWindow(BRect(20, 24, 400, 600), "Preview", B_DOCUMENT_WINDOW, 0)
{
	const float kButtonDistance = 15;
	const float kPageTextDistance = 10;
	const float kVerticalIndent = 7;
	const float kHorizontalIndent = 20;
	
	float top = kVerticalIndent;
	float left = kHorizontalIndent;
	float width, height;
		
	BRect r = Frame();
	r.right = r.IntegerWidth() - B_V_SCROLL_BAR_WIDTH; r.left = 0;
	r.bottom = r.IntegerHeight() - B_H_SCROLL_BAR_HEIGHT; r.top = 0;

	// add navigation and zoom buttons
	fFirst = new BButton(BRect(left, top, left+10, top+10), "First", "First Page", new BMessage(MSG_FIRST_PAGE));
	AddChild(fFirst);

	fPrev = new BButton(BRect(left, top, left+10, top+10), "Prev", "Previous Page", new BMessage(MSG_PREV_PAGE));
	AddChild(fPrev);
	fPrev->ResizeToPreferred();
	// remember size of largest button
	width = fPrev->Bounds().Width()+1;
	height = fPrev->Bounds().Height()+1;

	fFirst->ResizeTo(width, height);
	left = fFirst->Frame().right + kButtonDistance;

	// move Previous Page button after First Page button
	fPrev->MoveTo(left, top);	
	left = fPrev->Frame().right + kButtonDistance;
	
	fNext = new BButton(BRect(left, top, left+10, top+10), "Next", "Next Page", new BMessage(MSG_NEXT_PAGE));
	AddChild(fNext);
	fNext->ResizeTo(width, height);
	left = fNext->Frame().right + kButtonDistance;
	
	fLast = new BButton(BRect(left, top, left+10, top+10), "Last", "Last Page", new BMessage(MSG_LAST_PAGE));
	AddChild(fLast);
	fLast->ResizeTo(width, height);
	left = fLast->Frame().right + kPageTextDistance;
	
	// page number text
	fPageNumber = new BTextControl(BRect(left, top+3, left+10, top+10), "FindPage", "", "", new BMessage(MSG_FIND_PAGE));
	fPageNumber->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_RIGHT);
	int num;
	for (num = 0; num <= 255; num++) {
		fPageNumber->TextView()->DisallowChar(num);
	}
	for (num = 0; num <= 9; num++) {
		fPageNumber->TextView()->AllowChar('0' + num);
	}
	fPageNumber->TextView()-> SetMaxBytes(5);	
	AddChild(fPageNumber);
	fPageNumber->ResizeTo(be_plain_font->StringWidth("999999") + 10, height);
	left = fPageNumber->Frame().right + 5;
	
	// number of pages text
	fPageText = new BStringView(BRect(left, top, left + 100, top + 15), "pageText", "");
	AddChild(fPageText);
		// estimate max. width
	float pageTextWidth = fPageText->StringWidth("of 99999 Pages");
		// estimate height
	BFont font;
	fPageText->GetFont(&font);
	float pageTextHeight = font.Size() + 2;
		// resize to estimated size
	fPageText->ResizeTo(pageTextWidth, pageTextHeight);
	left += pageTextWidth + kPageTextDistance;
	fPageText->MoveBy(0, (height - font.Size()) / 2);
	
	
	fZoomIn = new BButton(BRect(left, top, left+10, top+10), "ZoomIn", "Zoom In", new BMessage(MSG_ZOOM_IN));
	AddChild(fZoomIn);
	fZoomIn->ResizeTo(width, height);
	left = fZoomIn->Frame().right + kButtonDistance;
	
	fZoomOut = new BButton(BRect(left, top, left+10, top+10), "ZoomOut", "Zoom Out", new BMessage(MSG_ZOOM_OUT));
	AddChild(fZoomOut);
	fZoomOut->ResizeTo(width, height);

	fButtonBarHeight = fZoomOut->Frame().bottom + 7;
	float bottomOfTopButtonBar = fButtonBarHeight;
	
	if (showOkAndCancelButtons) {
		// cancel printing if user closes the preview window
		SetUserQuitResult(B_ERROR);
		
		// add another button bar at bottom of window
		r.bottom -= fButtonBarHeight;
		// update the total height of both bars
		fButtonBarHeight *= 2;
		
		// right align buttons
		left = r.Width() - width - kHorizontalIndent;
		top = r.Height() + kVerticalIndent + B_H_SCROLL_BAR_HEIGHT;
		
		left -= width + kButtonDistance;		
		BButton *printJob = new BButton(BRect(left, top+2, left+10, top+12), "PrintJob", "Print", new BMessage(MSG_PRINT_JOB), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		printJob->MakeDefault(true);
		AddChild(printJob);
		printJob->ResizeTo(width+4, height+4);

		left += width + kButtonDistance;
		BButton *cancelJob = new BButton(BRect(left, top, left+10, top+10), "CancelJob", "Cancel", new BMessage(MSG_CANCEL_JOB), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		AddChild(cancelJob);
		cancelJob->ResizeTo(width, height);
	}		
	
	
		// add preview view		
	r.top = bottomOfTopButtonBar;
	fPreview = new PreviewView(jobFile, r);
	
	
	fPreviewScroller = new BScrollView("PreviewScroller", fPreview, B_FOLLOW_ALL, 0, true, true, B_FANCY_BORDER);
	fPreviewScroller->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fPreviewScroller);
	
	if (fPreview->InitCheck() == B_OK) {
		ResizeToPage(true);
		fPreview->FixScrollbars();
		UpdateControls();
	}
}

void PreviewWindow::ResizeToPage(bool allowShrinking) {
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
	
	if (!allowShrinking) {
		if (Bounds().Width() > width) width = Bounds().Width();
		if (Bounds().Height() > height) height = Bounds().Height();
	}
	ResizeTo(width, height);
}

void PreviewWindow::UpdateControls() {
	fFirst->SetEnabled(!fPreview->ShowsFirstPage());
	fPrev->SetEnabled(!fPreview->ShowsFirstPage());
	fNext->SetEnabled(!fPreview->ShowsLastPage());
	fLast->SetEnabled(!fPreview->ShowsLastPage());
	fZoomIn->SetEnabled(fPreview->CanZoomIn());
	fZoomOut->SetEnabled(fPreview->CanZoomOut());
	
	BString page;
	page << fPreview->CurrentPage();
	fPageNumber->SetText(page.String());	

	BString text;  
	text << "of " 
		<< fPreview->NumberOfPages();
	if (fPreview->NumberOfPages() == 1) {
		text << " Page";
	} else {
		text << " Pages";
	}
	fPageText->SetText(text.String());}

void PreviewWindow::MessageReceived(BMessage* m) {
	switch (m->what) {
		case MSG_FIRST_PAGE: 
			fPreview->ShowFirstPage();
			break;
		case MSG_NEXT_PAGE: 
			fPreview->ShowNextPage();
			break;
		case MSG_PREV_PAGE: 
			fPreview->ShowPrevPage();
			break;	
		case MSG_LAST_PAGE: 
			fPreview->ShowLastPage();
			break;
		case MSG_FIND_PAGE: 
			fPreview->ShowFindPage(atoi(fPageNumber->Text())) ;
			break;
		case MSG_ZOOM_IN: 
			fPreview->ZoomIn(); 
			ResizeToPage();
			break;
		case MSG_ZOOM_OUT: 
			fPreview->ZoomOut(); 
			ResizeToPage();
			break;
		case MSG_CANCEL_JOB: 
			Quit(B_ERROR);
			break;
		case MSG_PRINT_JOB: 
			Quit(B_OK);
			break;

		default:
			inherited::MessageReceived(m); return;
	}
	UpdateControls();
}

status_t PreviewWindow::Go() {
	status_t st = InitCheck();
	if (st == B_OK) {
		return inherited::Go();
	} else {
		Quit();
	}
	return st;
}
