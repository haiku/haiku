/*

Preview printer driver.

Copyright (c) 2001 OpenBeOS. 

Authors: 
	Philippe Houdoin
	Simon Gauvin	
	Michael Pfeiffer
	
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

#ifndef PRINTERDRIVER_H
#define PRINTERDRIVER_H

#include <AppKit.h>
#include <InterfaceKit.h>
#include "PrintTransport.h"
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
	inline BDataIO			*Transport()	{ return fPrintTransport.GetDataIO(); }
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
	
	// transport-related 
	PrintTransport          fPrintTransport;
};

#endif // #ifndef PRINTERDRIVER_H

