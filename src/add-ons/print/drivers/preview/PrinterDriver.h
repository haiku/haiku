/*
 * Copyright 2001-2007, Haiku. All rights reserved.
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

#include <AppKit.h>
#include <InterfaceKit.h>
#include "InterfaceUtils.h"

#ifndef ROUND_UP
	#define ROUND_UP(x, y) (((x) + (y) - 1) & ~((y) - 1))
#endif

#define MAX_INT32 ((int32)0x7fffffffL)

/* copied from PDFlib.h: */
#define a0_width	 (float) 2380.0
#define a0_height	 (float) 3368.0
#define a1_width	 (float) 1684.0
#define a1_height	 (float) 2380.0
#define a2_width	 (float) 1190.0
#define a2_height	 (float) 1684.0
#define a3_width	 (float) 842.0
#define a3_height	 (float) 1190.0
#define a4_width	 (float) 595.0
#define a4_height	 (float) 842.0
#define a5_width	 (float) 421.0
#define a5_height	 (float) 595.0
#define a6_width	 (float) 297.0
#define a6_height	 (float) 421.0
#define b5_width	 (float) 501.0
#define b5_height	 (float) 709.0
#define letter_width	 (float) 612.0
#define letter_height	 (float) 792.0
#define legal_width 	 (float) 612.0
#define legal_height 	 (float) 1008.0
#define ledger_width	 (float) 1224.0
#define ledger_height	 (float) 792.0
#define p11x17_width	 (float) 792.0
#define p11x17_height	 (float) 1224.0


/**
 * Class PrinterDriver
 */
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
	virtual BlockingWindow* NewPrinterSetupWindow(char* printerName);
	virtual BlockingWindow* NewPageSetupWindow(BMessage *setupMsg, const char *printerName);
	virtual BlockingWindow* NewJobSetupWindow(BMessage *setupMsg, const char *printerName);
	
	// configuration default methods
	virtual status_t 		PrinterSetup(char *printerName);
	virtual status_t 		PageSetup(BMessage *msg, const char *printerName = NULL);
	virtual status_t 		JobSetup(BMessage *msg, const char *printerName = NULL);
	virtual BMessage*       GetDefaultSettings();
	
	// accessors
	inline BFile			*JobFile()		{ return fJobFile; }
	inline BNode			*PrinterNode()	{ return fPrinterNode; }
	inline BMessage			*JobMsg()		{ return fJobMsg; }
	inline int32            Pass() const    { return fPass; }
	
	// publics status code
	typedef enum {
		PORTRAIT_ORIENTATION,
		LANDSCAPE_ORIENTATION
	} Orientation;


private:
	status_t Go(BlockingWindow* w);
	
	BFile					*fJobFile;
	BNode					*fPrinterNode;
	BMessage				*fJobMsg;

	volatile Orientation	fOrientation;
	
	bool					fPrinting;
	int32                   fPass;
};

#endif // #ifndef PRINTERDRIVER_H

