/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_PRINTSESSION_H
#define	_PRINTSESSION_H


#include <Picture.h>


class BFile;
class BView;

struct print_file_header {
	int32	version;
	int32	page_count;
	off_t	first_page;

	int32	_reserved[3];
} _PACKED;

struct _page_header_;


class BPrintJob {
public:
	// Values returned by PrinterType()
	enum {
		B_BW_PRINTER = 0,
		B_COLOR_PRINTER
	};


								BPrintJob(const char* name);
	virtual						~BPrintJob();

			void				BeginJob();
			void				CommitJob();
			status_t			ConfigJob();
			void				CancelJob();

			status_t			ConfigPage();
			void				SpoolPage();

			bool				CanContinue();

	virtual	void				DrawView(BView* view, BRect rect,
									BPoint where);

			BMessage*			Settings(); // TODO: const
			void				SetSettings(BMessage* archive);
			bool				IsSettingsMessageValid(
									BMessage* archive) const;

			BRect				PaperRect();
			BRect				PrintableRect();
			void				GetResolution(int32* xDPI, int32* yDPI);

			int32				FirstPage(); // TODO: const
			int32				LastPage(); // TODO: const
			int32				PrinterType(void* type = NULL) const;


private:
	// FBC padding and forbidden methods
	virtual void				_ReservedPrintJob1();
	virtual void				_ReservedPrintJob2();
	virtual void				_ReservedPrintJob3();
	virtual void				_ReservedPrintJob4();

								BPrintJob(const BPrintJob& other);
			BPrintJob&			operator=(const BPrintJob& other);

private:
			void				_RecurseView(BView* view, BPoint origin,
									BPicture* picture, BRect rect);
			void				_GetMangledName(char* buffer,
									size_t bufferSize) const;
			void				_HandlePageSetup(BMessage* setup);
			bool				_HandlePrintSetup(BMessage* setup);

			void				_NewPage();
			void				_EndLastPage();

			void				_AddSetupSpec();
			void				_AddPicture(BPicture& picture, BRect& rect,
									BPoint& where);

			char*				_GetCurrentPrinterName() const;
			void				_LoadDefaultSettings();

private:
			char*				fPrintJobName;

			int32				_unused;

			BFile*				fSpoolFile;
			print_file_header	fSpoolFileHeader;
			BRect				fPaperSize;
			BRect				fUsableSize;
			status_t			fError;
			char				fSpoolFileName[256];
			BMessage*			fSetupMessage;
			BMessage*			fDefaultSetupMessage;
			char				fAbort;
			int32				fFirstPage;
			int32				fLastPage;
			short				fXResolution;
			short				fYResolution;
			_page_header_*		fCurrentPageHeader;
			off_t				fCurrentPageHeaderOffset;

			uint32				_reserved[2];
};

#endif // _PRINTSESSION_H
