/*
 * Copyright 2002-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 *		Hartmut Reh
 *		julun <host.haiku@gmx.de>
 */
#include "BlockingWindow.h"
#include "JobData.h"
#include "PrintJobReader.h"


#include <View.h>


class BButton;
class BFile;
class BMessage;
class BPicture;
class BScrollView;
class BStringView;
class BTextControl;


// #pragma mark - PreviewPage


class PreviewPage {
public:
				PreviewPage(int32 page, PrintJobPage* pjp);
				~PreviewPage();

	void		Draw(BView* view, const BRect& printRect);
	status_t	InitCheck() const;
	int32 		Page() const { return fPage; }

private:
	int32		fPage;
	status_t	fStatus;
	int32		fNumberOfPictures;

	BRect*		fRects;
	BPoint*		fPoints;
	BPicture*	fPictures;
};


// #pragma mark - PreviewView


class PreviewView : public BView {
public:
							PreviewView(BFile* jobFile, BRect rect);
	virtual					~PreviewView();

	virtual void			Show();
	virtual void			Hide();
	virtual void			Draw(BRect r);
	virtual void			FrameResized(float width, float height);
	virtual void			MouseDown(BPoint point);
	virtual void			MouseMoved(BPoint point, uint32 transit,
								const BMessage* message);
	virtual void			MouseUp(BPoint point);
	virtual void			KeyDown(const char* bytes, int32 numBytes);

			void			ShowFirstPage();
			void			ShowPrevPage();
			void			ShowNextPage();
			void			ShowLastPage();
			bool			ShowsFirstPage() const;
			bool			ShowsLastPage() const;
			void			ShowFindPage(int32 page);

			void			ZoomIn();
			bool			CanZoomIn() const;
			void			ZoomOut();
			bool			CanZoomOut() const;

			void			FixScrollbars();
			BRect			ViewRect() const;
			status_t		InitCheck() const;
			int32			NumberOfPages() const;
			int32			CurrentPage() const { return fPage + 1; }

private:
			BRect			_PaperRect() const;
			float			_ZoomFactor() const;
			BRect			_PrintableRect() const;

			void			_LoadPage(int32 page);
			bool			_IsPageValid() const;
			bool			_IsPageLoaded(int32 page) const;

			BRect			_ContentRect() const;
			void			_DrawPageFrame(BRect rect);
			void			_DrawPage(BRect updateRect);
			void			_DrawMarginFrame(BRect rect);

			int32			_GetPageNumber(int32 index) const;

private:
			int32			fPage;
			int32			fZoom;
			PrintJobReader	fReader;
			bool			fReverse;
			BRect			fPaperRect;
			BRect			fPrintableRect;
			bool			fTracking;
			bool			fInsideView;
			BPoint			fScrollStart;
			int32			fNumberOfPages;
			int32			fNumberOfPagesPerPage;
			PreviewPage*	fCachedPage;

			JobData::Orientation	fOrientation;
			JobData::PageSelection	fPageSelection;
};


// #pragma mark - PreviewWindow


class PreviewWindow : public BlockingWindow {
public:
							PreviewWindow(BFile* jobFile,
								bool showOkAndCancelButtons = false);

	virtual void			MessageReceived(BMessage* m);

			status_t		Go();
			status_t		InitCheck() const { return fPreview->InitCheck(); }


private:
			void			_ResizeToPage();
			void			_UpdateControls();

private:
			BButton*		fFirst;
			BButton*		fNext;
			BButton*		fPrev;
			BButton*		fLast;
			BButton*		fZoomIn;
			BButton*		fZoomOut;
			BTextControl*	fPageNumber;
			BStringView*	fPageText;
			PreviewView*	fPreview;
			BScrollView*	fPreviewScroller;
			float			fButtonBarHeight;

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
};
