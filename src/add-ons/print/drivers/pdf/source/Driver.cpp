/*

PDF Writer printer driver.

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


#include <stdio.h>
#include <string.h>

#include <StorageKit.h>

#include "Driver.h"
#include "PDFWriter.h"
#include "PrinterSettings.h"

static PrinterDriver *instanciate_driver(BNode *spoolDir);

//  ======== For testing only ==================

BMessage* 
take_job(BFile *spoolFile, BNode *spoolDir, BMessage *msg) 
{
	PrinterDriver *driver;
	
	driver = instanciate_driver(spoolDir);
	if (driver->PrintJob(spoolFile, spoolDir, msg) == B_OK) {
		msg = new BMessage('okok');
	} else {	
		msg = new BMessage('baad');
	}
	delete driver;
			
	return msg;
}


// --------------------------------------------------
BMessage* 
config_page(BNode *spoolDir, BMessage *msg) 
{
	BMessage		*pagesetupMsg;
	PrinterDriver	*driver;
	const char		*printerName;
	char			buffer[B_ATTR_NAME_LENGTH+1];

	pagesetupMsg = new BMessage(*msg);
	
	PrinterSettings::Update(spoolDir, pagesetupMsg);

	// retrieve the printer (spool) name.
	printerName = NULL;
	if (spoolDir->ReadAttr("Printer Name", B_STRING_TYPE, 1, buffer, B_ATTR_NAME_LENGTH+1) > 0) {
		printerName = buffer;
	}

	driver = instanciate_driver(spoolDir);
	if (driver->PageSetup(pagesetupMsg, printerName) == B_OK) {
		pagesetupMsg->what = 'okok';
		PrinterSettings::Update(spoolDir, pagesetupMsg);
	} else {
		delete pagesetupMsg;
		pagesetupMsg = NULL;
	}
		
	delete driver;
	
	return pagesetupMsg;
}


// --------------------------------------------------
BMessage* 
config_job(BNode *spoolDir, BMessage *msg)
{
	BMessage		*jobsetupMsg;
	PrinterDriver	*driver;
	const char		*printerName;
	char			buffer[B_ATTR_NAME_LENGTH+1];

	jobsetupMsg = new BMessage(*msg);

	PrinterSettings::Update(spoolDir, jobsetupMsg);

	// retrieve the printer (spool) name.
	printerName = NULL;
	if (spoolDir->ReadAttr("Printer Name", B_STRING_TYPE, 1, buffer, B_ATTR_NAME_LENGTH+1) > 0) {
		printerName = buffer;
	}
	driver = instanciate_driver(spoolDir);
	if (driver->JobSetup(jobsetupMsg, printerName) == B_OK) {
		jobsetupMsg->what = 'okok';
		PrinterSettings::Update(spoolDir, jobsetupMsg);
	} else {
		delete jobsetupMsg;
		jobsetupMsg = NULL;
	}

	delete driver;
	
	return jobsetupMsg;
}


// --------------------------------------------------
char* 
add_printer(char *printerName)
{
	return printerName; 
}


// --------------------------------------------------
static PrinterDriver* 
instanciate_driver(BNode *spoolDir)
{
	return new PDFWriter();
}

/**
 * default_settings  
 *
 * @param BNode* printer spool directory
 * @return BMessage* the settings
 */
BMessage* 
default_settings(BNode* printer)
{
	BMessage *msg = new BMessage();

	PrinterSettings::Update(printer, msg);
	
	return msg;
}

