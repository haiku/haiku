/*
 * Copyright 2001-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin
 *		Michael Pfeiffer
 *		Dr. Hartmut Reh
 */

#ifndef PRINTERDRIVER_H
#define PRINTERDRIVER_H


#include "BlockingWindow.h"


class BFile;
class BlockingWindow;
class BNode;
class BMessage;


class PrinterDriver
{
public:
	// constructors / destructor
							PrinterDriver(BNode* printerNode);
	virtual					~PrinterDriver();

	void StopPrinting();

	virtual status_t 		PrintJob(BFile *jobFile, BMessage *jobMsg);
	virtual status_t        BeginJob();
	virtual status_t		PrintPage(int32 pageNumber, int32 pageCount);
	virtual status_t        EndJob();

	// configuration window getters
	virtual BlockingWindow* NewPageSetupWindow(BMessage *setupMsg, const char *printerName);
	virtual BlockingWindow* NewJobSetupWindow(BMessage *setupMsg, const char *printerName);

	// configuration default methods
	virtual status_t 		PageSetup(BMessage *msg, const char *printerName = NULL);
	virtual status_t 		JobSetup(BMessage *msg, const char *printerName = NULL);
	virtual BMessage*       GetDefaultSettings();

	// accessors
	inline BFile			*JobFile()		{ return fJobFile; }
	inline BNode			*PrinterNode()	{ return fPrinterNode; }

	// publics status code
	typedef enum {
		PORTRAIT_ORIENTATION,
		LANDSCAPE_ORIENTATION
	} Orientation;


private:
	bool					fPrinting;
	BFile					*fJobFile;
	BNode					*fPrinterNode;

	volatile Orientation	fOrientation;
};

#endif

