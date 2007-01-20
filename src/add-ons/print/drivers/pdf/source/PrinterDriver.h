/*

PDF Writer printer driver.

Copyright (c) 2001-2003 OpenBeOS. 

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

#ifndef _PRINTER_DRIVER_H
#define _PRINTER_DRIVER_H

#include <AppKit.h>
#include <InterfaceKit.h>

#include "PrintTransport.h"

#ifndef ROUND_UP
	#define ROUND_UP(x, y) (((x) + (y) - 1) & ~((y) - 1))
#endif

#define MAX_INT32 ((int32)0x7fffffffL)

// transport add-on calls definition 
extern "C" {
	typedef BDataIO *(*init_transport_proc)(BMessage *);
	typedef void 	(*exit_transport_proc)(void);
};	


// close StatusWindow after pdf generation
enum CloseOption {
	kAlways,
	kNoErrors,
	kNoErrorsOrWarnings,
	kNoErrorsWarningsOrInfo,
	kNever,
};
	


/**
 * Class PrinterDriver
 */
class PrinterDriver 
{
public:
	// constructors / destructor
							PrinterDriver();
	virtual					~PrinterDriver();
	
	void StopPrinting();

	virtual status_t 		PrintJob(BFile *jobFile, BNode *printerNode, BMessage *jobMsg);
	virtual status_t        BeginJob();
	virtual status_t		PrintPage(int32 pageNumber, int32 pageCount);
	virtual status_t        EndJob();

	// configuration default methods
	virtual status_t 		PrinterSetup(char *printerName);
	virtual status_t 		PageSetup(BMessage *msg, const char *printerName = NULL);
	virtual status_t 		JobSetup(BMessage *msg, const char *printerName = NULL);
	
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
	BFile					*fJobFile;
	BNode					*fPrinterNode;
	BMessage				*fJobMsg;

	volatile Orientation	fOrientation;
	
	bool					fPrinting;
	int32                   fPass;
	
	// transport-related 
	PrintTransport          fPrintTransport;
};

#endif // _PRINTER_DRIVER_H
