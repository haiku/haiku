/*
 * Copyright 2002-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 *		Hartmut Reh
 *		julun <host.haiku@gmx.de>
 */

#include "Preview.h"

#include "GraphicsDriver.h"
#include "PrintUtils.h"


#include <math.h>
#include <stdlib.h>


#include <Application.h>
#include <Button.h>
#include <Debug.h>
#include <Region.h>
#include <Screen.h>
#include <String.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>


// #pragma mark - PreviewPage


PreviewPage::PreviewPage(int32 page, PrintJobPage* pjp)
	: fPage(page)
	, fStatus(B_ERROR)
	, fNumberOfPictures(0)
	, fRects(NULL)
	, fPoints(NULL)
	, fPictures(NULL)
{
	fNumberOfPictures = pjp->NumberOfPictures();

	fRects = new BRect[fNumberOfPictures];
	fPoints = new BPoint[fNumberOfPictures];
	fPictures = new BPicture[fNumberOfPictures];

	for (int32 i = 0; i < fNumberOfPictures; ++i) {
		fStatus = pjp->NextPicture(fPictures[i], fPoints[i], fRects[i]);
		if (fStatus != B_OK)
			break;
	}
}


PreviewPage::~PreviewPage()
 {
	delete [] fRects;
	delete [] fPoints;
	delete [] fPictures;
}


status_t
PreviewPage::InitCheck() const
{
	return fStatus;
}


void
PreviewPage::Draw(BView* view, const BRect& printRect)
{
	ASSERT(fStatus == B_OK);
	for (int32 i = 0; i < fNumberOfPictures; i++)
		view->DrawPicture(&fPictures[i], printRect.LeftTop() + fPoints[i]);
}


// #pragma mark - PreviewView


namespace {

const float kPreviewTopMargin		= 10.0;
const float kPreviewBottomMargin	= 30.0;
const float kPreviewLeftMargin		= 10.0;
const float kPreviewRightMargin		= 30.0;


// TODO share constant with JobData
const char *kJDOrientation		= "orientation";
const char *kJDNup				= "JJJJ_nup";
const char *kJDReverse			= "JJJJ_reverse";
const char* kJDPageSelection	= "JJJJ_page_selection";


const uint8 ZOOM_IN[] = { 16, 1, 6, 6, 0, 0, 15, 128, 48, 96, 32, 32, 66, 16,
	66, 16, 79, 144, 66, 16, 66, 16, 32, 32, 48, 112, 15, 184, 0, 28, 0, 14, 0,
	6, 0, 0, 15, 128, 63, 224, 127, 240, 127, 240, 255, 248, 255, 248, 255, 248,
	255, 248, 255, 248, 127, 248, 127, 248, 63, 252, 15, 254, 0, 31, 0, 15, 0, 7 };


const uint8 ZOOM_OUT[] = { 16, 1, 6, 6, 0, 0, 15, 128, 48, 96, 32, 32, 64, 16,
	64, 16, 79, 144, 64, 16, 64, 16, 32, 32, 48, 112, 15, 184, 0, 28, 0, 14, 0,
	6, 0, 0, 15, 128, 63, 224, 127, 240, 127, 240, 255, 248, 255, 248, 255, 248,
	255, 248, 255, 248, 127, 248, 127, 248, 63, 252, 15, 254, 0, 31, 0, 15, 0, 7 };


BRect
RotateRect(const BRect& rect)
{
	return BRect(rect.top, rect.left, rect.bottom, rect.right);
}


BRect
ScaleDown(BRect rect, float factor)
{
	rect.left /= factor;
	rect.top /= factor;
	rect.right /= factor;
	rect.bottom /= factor;
	return rect;
}


BPoint
CalulateOffset(int32 numberOfPagesPerPage, int32 index,
	JobData::Orientation orientation, BRect printableRect)
{
	BPoint offset(0.0, 0.0);
	if (numberOfPagesPerPage == 1)
		return offset;

	float width  = printableRect.Width();
	float height = printableRect.Height();

	switch (numberOfPagesPerPage) {
		case 2: {
			if (index == 1) {
				if (JobData::kPortrait == orientation)
					offset.x = width;
				else
					offset.y = height;
			}
		}	break;

		case 8: {
			if (JobData::kPortrait == orientation) {
				offset.x = width  * (index / 2);
				offset.y = height * (index % 2);
			} else {
				offset.x = width  * (index % 2);
				offset.y = height * (index / 2);
			}
		}	break;

		case 32: {
			if (JobData::kPortrait == orientation) {
				offset.x = width  * (index / 4);
				offset.y = height * (index % 4);
			} else {
				offset.x = width  * (index % 4);
				offset.y = height * (index / 4);
			}
		}	break;

		case 4: {
		case 9:
		case 16:
		case 25:
		case 36:
		case 49:
		case 64:
		case 81:
		case 100:
		case 121:
			int32 value = int32(sqrt(double(numberOfPagesPerPage)));
			offset.x = width  * (index % value);
			offset.y = height * (index / value);
		}	break;
	}
	return offset;
}

}


PreviewView::PreviewView(BFile* jobFile, BRect rect)
	: BView(rect, "PreviewView", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
	, fPage(0)
	, fZoom(0)
	, fReader(jobFile)
	, fReverse(false)
	, fPaperRect(BRect())
	, fPrintableRect(BRect())
	, fTracking(false)
	, fInsideView(true)
	, fScrollStart(0.0, 0.0)
	, fNumberOfPages(1)
	, fNumberOfPagesPerPage(1)
	, fCachedPage(NULL)
	, fOrientation(JobData::kPortrait)
	, fPageSelection(JobData::kAllPages)
{
	int32 value32;
	if (fReader.JobSettings()->FindInt32(kJDOrientation, &value32) == B_OK)
		fOrientation = (JobData::Orientation)value32;

	if (fReader.JobSettings()->FindInt32(kJDPageSelection, &value32) == B_OK)
		fPageSelection = (JobData::PageSelection)value32;

	bool value;
	if (fReader.JobSettings()->FindBool(kJDReverse, &value) == B_OK)
		fReverse = value;

	if (fReader.JobSettings()->FindInt32(kJDNup, &value32) == B_OK)
		fNumberOfPagesPerPage = value32;

	fNumberOfPages = (fReader.NumberOfPages() + fNumberOfPagesPerPage - 1)
		/ fNumberOfPagesPerPage;

	if (fPageSelection == JobData::kOddNumberedPages)
		fNumberOfPages = (fNumberOfPages + 1) / 2;
	else if (fPageSelection == JobData::kEvenNumberedPages)
		fNumberOfPages /= 2;

	fPaperRect = fReader.PaperRect();
	fPrintableRect = fReader.PrintableRect();
	switch (fNumberOfPagesPerPage) {
		case 2:
		case 8:
		case 32:
		case 128: {
			fPaperRect = RotateRect(fPaperRect);
			fPrintableRect = RotateRect(fPrintableRect);
		}	break;
	}
}


PreviewView::~PreviewView()
{
	delete fCachedPage;
}


void
PreviewView::Show()
{
	BView::Show();
	be_app->SetCursor(ZOOM_IN);
}


void
PreviewView::Hide()
{
	be_app->SetCursor(B_HAND_CURSOR);
	BView::Hide();
}


void
PreviewView::Draw(BRect rect)
{
	if (fReader.InitCheck() == B_OK) {
		_DrawPageFrame(rect);
		_DrawPage(rect);
		_DrawMarginFrame(rect);
	}
}


void
PreviewView::FrameResized(float width, float height)
{
	Invalidate();
	FixScrollbars();
}


void
PreviewView::MouseDown(BPoint point)
{
	MakeFocus(true);
	BMessage *message = Window()->CurrentMessage();

	int32 button;
	if (message && message->FindInt32("buttons", &button) == B_OK) {
		if (button == B_PRIMARY_MOUSE_BUTTON) {
			int32 modifier;
			if (message->FindInt32("modifiers", &modifier) == B_OK) {
				if (modifier & B_SHIFT_KEY)
					ZoomOut();
				else
					ZoomIn();
			}
		}

		if (button == B_SECONDARY_MOUSE_BUTTON) {
			fTracking = true;
			be_app->SetCursor(B_HAND_CURSOR);
			SetMouseEventMask(B_POINTER_EVENTS,
				B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
			fScrollStart = Bounds().LeftTop() + ConvertToScreen(point);
		}
	}
}


void
PreviewView::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if (fTracking) {
		uint32 button;
		GetMouse(&point, &button, false);
		point = fScrollStart - ConvertToScreen(point);

		float hMin, hMax;
		BScrollBar *hBar = ScrollBar(B_HORIZONTAL);
		hBar->GetRange(&hMin, &hMax);

		float vMin, vMax;
		BScrollBar *vBar = ScrollBar(B_VERTICAL);
		vBar->GetRange(&vMin, &vMax);

		if (point.x < 0.0) point.x = 0.0;
		if (point.y < 0.0) point.y = 0.0;
		if (point.x > hMax) point.x = hMax;
		if (point.y > vMax) point.y = vMax;

		ScrollTo(point);
	} else {
		switch (transit) {
			case B_ENTERED_VIEW: {
				fInsideView = true;
				be_app->SetCursor(ZOOM_IN);
			}	break;

			case B_EXITED_VIEW: {
				fInsideView = false;
				be_app->SetCursor(B_HAND_CURSOR);
			}	break;

			default: {
				if (modifiers() & B_SHIFT_KEY)
					be_app->SetCursor(ZOOM_OUT);
				else
					be_app->SetCursor(ZOOM_IN);
			}	break;
		}
	}
}


void
PreviewView::MouseUp(BPoint point)
{
	(void)point;
	fTracking = false;
	fScrollStart.Set(0.0, 0.0);
	if (fInsideView && ((modifiers() & B_SHIFT_KEY) == 0))
		be_app->SetCursor(ZOOM_IN);
}


void
PreviewView::KeyDown(const char* bytes, int32 numBytes)
{
	MakeFocus(true);
	switch (bytes[0]) {
		case '-': {
			if (modifiers() & B_CONTROL_KEY)
				ZoomOut();
		}	break;

		case '+' : {
			if (modifiers() & B_CONTROL_KEY)
				ZoomIn();
		}	break;

		default: {
			BView::KeyDown(bytes, numBytes);
		}	break;
	}
}


void
PreviewView::ShowFirstPage()
{
	if (!ShowsFirstPage()) {
		fPage = 0;
		Invalidate();
	}
}


void
PreviewView::ShowPrevPage()
{
	if (!ShowsFirstPage()) {
		fPage--;
		Invalidate();
	}
}


void
PreviewView::ShowNextPage()
{
	if (!ShowsLastPage()) {
		fPage++;
		Invalidate();
	}
}


void
PreviewView::ShowLastPage()
{
	if (!ShowsLastPage()) {
		fPage = NumberOfPages()-1;
		Invalidate();
	}
}


bool
PreviewView::ShowsFirstPage() const
{
	return fPage == 0;
}


bool
PreviewView::ShowsLastPage() const
{
	return fPage == NumberOfPages() - 1;
}


void
PreviewView::ShowFindPage(int32 page)
{
	page--;

	if (page < 0) {
		page = 0;
	} else if (page > (NumberOfPages()-1)) {
		page = NumberOfPages()-1;
	}

	fPage = page;
	Invalidate();
}


void
PreviewView::ZoomIn()
{
	if (CanZoomIn()) {
		fZoom++;
		FixScrollbars();
		Invalidate();
	}
}


bool
PreviewView::CanZoomIn() const
{
	return fZoom < 4;
}


void
PreviewView::ZoomOut()
{
	if (CanZoomOut()) {
		fZoom--;
		FixScrollbars();
		Invalidate();
	}
}


bool
PreviewView::CanZoomOut() const
{
	return fZoom > -2;
}


void
PreviewView::FixScrollbars()
{
	float width = _PaperRect().Width() + kPreviewLeftMargin + kPreviewRightMargin;
	float height = _PaperRect().Height() + kPreviewTopMargin + kPreviewBottomMargin;

	BRect frame(Bounds());
	float x = width - frame.Width();
	if (x < 0.0)
		x = 0.0;

	float y = height - frame.Height();
	if (y < 0.0)
		y = 0.0;

	BScrollBar * scroll = ScrollBar(B_HORIZONTAL);
	scroll->SetRange(0.0, x);
	scroll->SetProportion((width - x) / width);
	float bigStep = frame.Width() - 2;
	float smallStep = bigStep / 10.;
	scroll->SetSteps(smallStep, bigStep);

	scroll = ScrollBar(B_VERTICAL);
	scroll->SetRange(0.0, y);
	scroll->SetProportion((height - y) / height);
	bigStep = frame.Height() - 2;
	smallStep = bigStep / 10.;
	scroll->SetSteps(smallStep, bigStep);
}


BRect
PreviewView::ViewRect() const
{
	BRect r(_PaperRect());
	r.right += kPreviewLeftMargin + kPreviewRightMargin;
	r.bottom += kPreviewTopMargin + kPreviewBottomMargin;
	return r;
}


status_t
PreviewView::InitCheck() const
{
	return fReader.InitCheck();
}


int32
PreviewView::NumberOfPages() const
{
	return fNumberOfPages;
}


BRect
PreviewView::_PaperRect() const
{
	return ScaleRect(fPaperRect, _ZoomFactor());
}


float
PreviewView::_ZoomFactor() const
{
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


BRect
PreviewView::_PrintableRect() const
{
	return ScaleRect(fPrintableRect, _ZoomFactor());
}


bool
PreviewView::_IsPageValid() const
{
	return fCachedPage && fCachedPage->InitCheck() == B_OK;
}


void
PreviewView::_LoadPage(int32 page)
{
	delete fCachedPage;
	fCachedPage = NULL;

	PrintJobPage pjp;
	if (fReader.GetPage(page, pjp) == B_OK)
		fCachedPage = new PreviewPage(page, &pjp);
}


bool
PreviewView::_IsPageLoaded(int32 page) const
{
	return fCachedPage != NULL && fCachedPage->Page() == page;
}


BRect
PreviewView::_ContentRect() const
{
	float offsetX = kPreviewLeftMargin;
	float offsetY = kPreviewTopMargin;

	BRect rect = Bounds();
	BRect paperRect = _PaperRect();

	float min, max;
	ScrollBar(B_HORIZONTAL)->GetRange(&min, &max);
	if (min == max) {
		if ((paperRect.right + 2 * offsetX) < rect.right)
			offsetX = (rect.right - (paperRect.right + 2 * offsetX)) / 2;
	}

	ScrollBar(B_VERTICAL)->GetRange(&min, &max);
	if (min == max) {
		if ((paperRect.bottom + 2 * offsetY) < rect.bottom)
			offsetY = (rect.bottom - (paperRect.bottom + 2 * offsetY)) / 2;
	}

	paperRect.OffsetTo(offsetX, offsetY);
	return paperRect;
}


void
PreviewView::_DrawPageFrame(BRect rect)
{
	const float kShadowIndent = 3;
	const float kShadowWidth = 3;

	const rgb_color frameColor = { 0, 0, 0, 0 };
	const rgb_color shadowColor = { 90, 90, 90, 0 };

	PushState();

	// draw page border around page
	BRect r(_ContentRect().InsetByCopy(-1, -1));

	SetHighColor(frameColor);
	StrokeRect(r);

	// draw page shadow
	SetHighColor(shadowColor);

	float x = r.right + 1;
	float right = x + kShadowWidth;
	float bottom = r.bottom + 1 + kShadowWidth;
	float y = r.top + kShadowIndent;
	FillRect(BRect(x, y, right, bottom));

	x = r.left + kShadowIndent;
	y = r.bottom  + 1;
	FillRect(BRect(x, y, r.right, bottom));

	PopState();
}



void PreviewView::_DrawPage(BRect rect)
{
	BRect printRect(_PrintableRect());
	switch (fNumberOfPagesPerPage) {
		case 2:
		case 8:
		case 32:
		case 128: {
			printRect = RotateRect(printRect);
		}	break;
	}
	printRect.OffsetBy(_ContentRect().LeftTop());

	BPoint scalingXY = GraphicsDriver::GetScale(fNumberOfPagesPerPage, printRect, 100.0);
	float scaling = min_c(scalingXY.x, scalingXY.y);

	printRect = ScaleDown(printRect, _ZoomFactor() * scaling);
	BRect clipRect(ScaleRect(printRect, scaling));

	for (int32 index = 0; index < fNumberOfPagesPerPage; ++index) {
		int32 pageNumber = _GetPageNumber(index);
		if (pageNumber < 0)
			continue;

		if (!_IsPageLoaded(pageNumber))
			_LoadPage(pageNumber);

		if (!_IsPageValid())
			continue;

		BPoint offset(CalulateOffset(fNumberOfPagesPerPage, index, fOrientation,
			clipRect));

		clipRect.OffsetTo(printRect.LeftTop());
		clipRect.OffsetBy(offset);

		BRegion clip(clipRect);
		ConstrainClippingRegion(&clip);

		SetScale(_ZoomFactor() * scaling);

		fCachedPage->Draw(this, printRect.OffsetByCopy(offset));

		if (fNumberOfPagesPerPage > 1)
			StrokeRect(clipRect.InsetByCopy(1.0, 1.0), B_MIXED_COLORS);

		SetScale(1.0);

		ConstrainClippingRegion(NULL);
	}
}


void
PreviewView::_DrawMarginFrame(BRect rect)
{
	BRect paperRect(_ContentRect());
	BRect printRect(_PrintableRect());
	printRect.OffsetBy(paperRect.LeftTop());

	const rgb_color highColor = HighColor();
	const rgb_color white = { 255, 255, 255, 255 };

	SetHighColor(white);

	FillRect(BRect(paperRect.left, paperRect.top, printRect.left
		, paperRect.bottom));
	FillRect(BRect(paperRect.left, paperRect.top, paperRect.right
		, printRect.top));
	FillRect(BRect(printRect.right, paperRect.top, paperRect.right
		, paperRect.bottom));
	FillRect(BRect(paperRect.left, printRect.bottom, paperRect.right
		, paperRect.bottom));

	SetHighColor(highColor);

	BeginLineArray(4);

	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	StrokeLine(BPoint(printRect.left, paperRect.top),
		BPoint(printRect.left, paperRect.bottom), B_MIXED_COLORS);
	StrokeLine(BPoint(printRect.right, paperRect.top),
		BPoint(printRect.right, paperRect.bottom), B_MIXED_COLORS);
	StrokeLine(BPoint(paperRect.left, printRect.top),
		BPoint(paperRect.right, printRect.top), B_MIXED_COLORS);
	StrokeLine(BPoint(paperRect.left, printRect.bottom),
		BPoint(paperRect.right, printRect.bottom), B_MIXED_COLORS);

	EndLineArray();
}



int32 PreviewView::_GetPageNumber(int32 index) const
{
	int32 page = fPage;
	if (fReverse)
		page = fNumberOfPages - fPage - 1;

	if (fPageSelection == JobData::kOddNumberedPages)
		page *= 2; // 0, 2, 4, ...
	else if (fPageSelection == JobData::kEvenNumberedPages)
		page = 2 * page + 1; // 1, 3, 5, ...

	return page * fNumberOfPagesPerPage + index;
}


// #pragma mark - PreviewWindow


PreviewWindow::PreviewWindow(BFile* jobFile, bool showOkAndCancelButtons)
	: BlockingWindow(BRect(20, 24, 400, 600), "Preview", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS)
	, fButtonBarHeight(0.0)
{
	BRect bounds(Bounds());

	BView* panel = new BBox(Bounds(), "top_panel", B_FOLLOW_ALL,
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
					B_PLAIN_BORDER);
	AddChild(panel);

	bounds.OffsetBy(10.0, 10.0);

	fFirst = new BButton(bounds, "first", "First Page", new BMessage(MSG_FIRST_PAGE));
	panel->AddChild(fFirst);
	fFirst->ResizeToPreferred();

	bounds.OffsetBy(fFirst->Bounds().Width() + 10.0, 0.0);
	fPrev = new BButton(bounds, "previous", "Previous Page", new BMessage(MSG_PREV_PAGE));
	panel->AddChild(fPrev);
	fPrev->ResizeToPreferred();

	bounds.OffsetBy(fPrev->Bounds().Width() + 10.0, 0.0);
	fNext = new BButton(bounds, "next", "Next Page", new BMessage(MSG_NEXT_PAGE));
	panel->AddChild(fNext);
	fNext->ResizeToPreferred();

	bounds.OffsetBy(fNext->Bounds().Width() + 10.0, 0.0);
	fLast = new BButton(bounds, "last", "Last Page", new BMessage(MSG_LAST_PAGE));
	panel->AddChild(fLast);
	fLast->ResizeToPreferred();

	bounds = fLast->Frame();
	bounds.OffsetBy(fLast->Bounds().Width() + 10.0, 0.0);
	fPageNumber = new BTextControl(bounds, "numOfPage", "99", "",
		new BMessage(MSG_FIND_PAGE));
	panel->AddChild(fPageNumber);
	fPageNumber->ResizeToPreferred();
	fPageNumber->SetDivider(0.0);
	fPageNumber->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_RIGHT);
	fPageNumber->MoveBy(0.0, bounds.Height() - fPageNumber->Bounds().Height());

	uint32 num;
	for (num = 0; num <= 255; num++)
		fPageNumber->TextView()->DisallowChar(num);

	for (num = 0; num <= 9; num++)
		fPageNumber->TextView()->AllowChar('0' + num);
	fPageNumber->TextView()-> SetMaxBytes(5);

	bounds.OffsetBy(fPageNumber->Bounds().Width() + 5.0, 0.0);
	fPageText = new BStringView(bounds, "pageText", "");
	panel->AddChild(fPageText);
	fPageText->ResizeTo(fPageText->StringWidth("of 99999 Pages"),
		fFirst->Bounds().Height());

	bounds.OffsetBy(fPageText->Bounds().Width() + 10.0, 0.0);
	fZoomIn = new BButton(bounds, "zoomIn", "Zoom In", new BMessage(MSG_ZOOM_IN));
	panel->AddChild(fZoomIn);
	fZoomIn->ResizeToPreferred();

	bounds.OffsetBy(fZoomIn->Bounds().Width() + 10.0, 0.0);
	fZoomOut = new BButton(bounds, "ZoomOut", "Zoom Out", new BMessage(MSG_ZOOM_OUT));
	panel->AddChild(fZoomOut);
	fZoomOut->ResizeToPreferred();

	fButtonBarHeight = fZoomOut->Frame().bottom + 10.0;

	bounds = Bounds();
	bounds.top = fButtonBarHeight;

	if (showOkAndCancelButtons) {
		// adjust preview height
		bounds.bottom -= fButtonBarHeight;
		// update the total height of both bars
		fButtonBarHeight *= 2;

		// cancel printing if user closes the preview window
		SetUserQuitResult(B_ERROR);

		BButton *printJob = new BButton(BRect(), "printJob", "Print",
			new BMessage(MSG_PRINT_JOB), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		panel->AddChild(printJob);
		printJob->ResizeToPreferred();
		printJob->MoveTo(bounds.right - (printJob->Bounds().Width() + 10.0),
			bounds.bottom + 10.0);

		BButton *cancelJob = new BButton(BRect(), "cancelJob", "Cancel",
			new BMessage(MSG_CANCEL_JOB), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		panel->AddChild(cancelJob);
		cancelJob->ResizeToPreferred();
		cancelJob->MoveTo(printJob->Frame().left - (10.0 + cancelJob->Bounds().Width()),
			bounds.bottom + 10.0);

		printJob->MakeDefault(true);
	}

	bounds.right -= B_V_SCROLL_BAR_WIDTH;
	bounds.bottom -= B_H_SCROLL_BAR_HEIGHT;

	fPreview = new PreviewView(jobFile, bounds);
	fPreviewScroller = new BScrollView("PreviewScroller", fPreview, B_FOLLOW_ALL,
		B_FRAME_EVENTS, true, true, B_FANCY_BORDER);
	fPreviewScroller->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	panel->AddChild(fPreviewScroller);

	if (fPreview->InitCheck() == B_OK) {
		_ResizeToPage();
		fPreview->FixScrollbars();
		_UpdateControls();
		fPreview->MakeFocus(true);
	}
}


void
PreviewWindow::MessageReceived(BMessage* m)
{
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
			break;

		case MSG_ZOOM_OUT:
			fPreview->ZoomOut();
			break;

		case B_MODIFIERS_CHANGED:
			fPreview->MouseMoved(BPoint(), B_INSIDE_VIEW, m);
			break;

		case MSG_CANCEL_JOB:
			Quit(B_ERROR);
			break;

		case MSG_PRINT_JOB:
			Quit(B_OK);
			break;

		default:
			BlockingWindow::MessageReceived(m);
			return;
	}
	_UpdateControls();
}


status_t
PreviewWindow::Go()
{
	status_t st = InitCheck();
	if (st == B_OK)
		return BlockingWindow::Go();

	be_app->SetCursor(B_HAND_CURSOR);
	Quit();
	return st;
}


void
PreviewWindow::_ResizeToPage()
 {
	BScreen screen;
	if (screen.Frame().right == 0.0)
		return;

	const float windowBorderWidth = 5;
	const float windowBorderHeight = 5;

	BRect rect(fPreview->ViewRect());
	float width = rect.Width() + 1 + B_V_SCROLL_BAR_WIDTH;
	float height = rect.Height() + 1 + fButtonBarHeight + B_H_SCROLL_BAR_HEIGHT;

	rect = screen.Frame();
	// dimensions so that window does not reach outside of screen
	float maxWidth = rect.Width() + 1 - windowBorderWidth - Frame().left;
	float maxHeight = rect.Height() + 1 - windowBorderHeight - Frame().top;

	// width so that all buttons are visible
	float minWidth = fZoomOut->Frame().right + 10;

	if (width < minWidth) width = minWidth;
	if (width > maxWidth) width = maxWidth;
	if (height > maxHeight) height = maxHeight;

	ResizeTo(width, height);
}


void
PreviewWindow::_UpdateControls()
{
	fFirst->SetEnabled(!fPreview->ShowsFirstPage());
	fPrev->SetEnabled(!fPreview->ShowsFirstPage());
	fNext->SetEnabled(!fPreview->ShowsLastPage());
	fLast->SetEnabled(!fPreview->ShowsLastPage());
	fZoomIn->SetEnabled(fPreview->CanZoomIn());
	fZoomOut->SetEnabled(fPreview->CanZoomOut());

	BString text;
	text << fPreview->CurrentPage();
	fPageNumber->SetText(text.String());

	text.SetTo("of ");
	text << fPreview->NumberOfPages() << " Page";
	if (fPreview->NumberOfPages() > 1)
		text.Append("s");
	fPageText->SetText(text.String());
}
