/*******************************************************************************
/
/	File:			PrintJob.h
/
/   Description:    BPrintJob runs a printing session.
/
/	Copyright 1996-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_PRINTSESSION_H
#define	_PRINTSESSION_H

#include <BeBuild.h>
#include <Picture.h>		/* For convenience */
#include <Rect.h>

class BView;

/*----------------------------------------------------------------*/
/*----- BPrintJob related structures -----------------------------*/

struct	print_file_header {
	int32	version;
	int32	page_count;
	off_t	first_page;
	int32	_reserved_3_;
	int32	_reserved_4_;
	int32	_reserved_5_;
};

struct _page_header_;

/*----------------------------------------------------------------*/
/*----- BPrintJob class ------------------------------------------*/

class BPrintJob {
public:

	enum 	// These values are returned by PrinterType()
	{
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

		BMessage	*Settings()	/* const */ ;
		void		SetSettings(BMessage *a_msg);
		bool		IsSettingsMessageValid(BMessage *a_msg) const;

		BRect		PaperRect();
		BRect		PrintableRect();
		void		GetResolution(int32 *xdpi, int32 *ydpi);

		int32		FirstPage()	/* const */ ;
		int32		LastPage()	/* const */ ;
		int32		PrinterType(void * = NULL) const;
		

/*----- Private or reserved -----------------------------------------*/
private:

virtual void		_ReservedPrintJob1();
virtual void		_ReservedPrintJob2();
virtual void		_ReservedPrintJob3();
virtual void		_ReservedPrintJob4();

					BPrintJob(const BPrintJob &);
		BPrintJob	&operator=(const BPrintJob &);

		void				RecurseView(BView *v, BPoint origin, BPicture *p, BRect r);
		void				MangleName(char *filename);
		void				HandlePageSetup(BMessage *setup);
		bool				HandlePrintSetup(BMessage *setup);

		void				NewPage();
		void				EndLastPage();

		void				AddSetupSpec();
		void				AddPicture(BPicture *picture, BRect *rect, BPoint where);

		char*				GetCurrentPrinterName() const;
		void				LoadDefaultSettings();

		char *				fPrintJobName;
		int32				fPageNumber;
		BFile *				fSpoolFile;
		print_file_header		fCurrentHeader;
		BRect				fPaperSize;
		BRect				fUsableSize;
		status_t			fError;
		char				fSpoolFileName[256];	
		BMessage			*fSetupMessage;
		BMessage			*fDefaultSetupMessage;
		char				fAbort;
		int32				fFirstPage;
		int32				fLastPage;
		short				fXResolution;
		short				fYResolution;
		_page_header_ *			fCurrentPageHeader;
		off_t				fCurrentPageHeaderOffset;
		uint32				_reserved[2];
};
 
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _PRINTSESSION_H */
