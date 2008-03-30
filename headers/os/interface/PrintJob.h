/*
 * Copyright 2008, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_PRINTSESSION_H
#define	_PRINTSESSION_H


#include <BeBuild.h>
#include <Picture.h>
#include <Rect.h>


class BFile;
class BView;


struct	print_file_header {
	int32	version;
	int32	page_count;
	off_t	first_page;
	int32	_reserved_3_;
	int32	_reserved_4_;
	int32	_reserved_5_;
};


struct _page_header_;


class BPrintJob {
public:

			enum {
						B_BW_PRINTER = 0,
						B_COLOR_PRINTER
			};


						BPrintJob(const char *job_name);
	virtual				~BPrintJob();

			void		BeginJob();
			void		CommitJob();
			status_t	ConfigJob();
			void		CancelJob();

			status_t	ConfigPage();
			void		SpoolPage();

			bool		CanContinue();

	virtual	void		DrawView(BView *a_view, BRect a_rect, BPoint where);

			BMessage *	Settings()	/* const */ ;
			void		SetSettings(BMessage *a_msg);
			bool		IsSettingsMessageValid(BMessage *a_msg) const;

			BRect		PaperRect();
			BRect		PrintableRect();
			void		GetResolution(int32 *xdpi, int32 *ydpi);

			int32		FirstPage()	/* const */ ;
			int32		LastPage()	/* const */ ;
			int32		PrinterType(void * = NULL) const;


private:
	virtual void		_ReservedPrintJob1();
	virtual void		_ReservedPrintJob2();
	virtual void		_ReservedPrintJob3();
	virtual void		_ReservedPrintJob4();

						BPrintJob(const BPrintJob &);
			BPrintJob	&operator=(const BPrintJob &);

			void		_RecurseView(BView *view, BPoint origin,
							BPicture *picture, BRect r);
			void		_GetMangledName(char *buffer, size_t bufferSize) const;
			void		_HandlePageSetup(BMessage *setup);
			bool		_HandlePrintSetup(BMessage *setup);

			void		_NewPage();
			void		_EndLastPage();

			void		_AddSetupSpec();
			void		_AddPicture(BPicture &picture, BRect &rect, BPoint &where);

			char *		_GetCurrentPrinterName() const;
			void		_LoadDefaultSettings();

private:
			char *				fPrintJobName;
			int32				fPageNumber;
			BFile *				fSpoolFile;
			print_file_header	fCurrentHeader;
			BRect				fPaperSize;
			BRect				fUsableSize;
			status_t			fError;
			char				fSpoolFileName[256];
			BMessage *			fSetupMessage;
			BMessage *			fDefaultSetupMessage;
			char				fAbort;
			int32				fFirstPage;
			int32				fLastPage;
			short				fXResolution;
			short				fYResolution;
			_page_header_ *		fCurrentPageHeader;
			off_t				fCurrentPageHeaderOffset;
			uint32				_reserved[2];
};


#endif	// _PRINTSESSION_H
