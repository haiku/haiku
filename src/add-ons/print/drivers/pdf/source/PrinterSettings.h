/*

PrinterSettings.h

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

#ifndef PRINTER_SETTINGS_H
#define PRINTER_SETTINGS_H 

#include <StorageKit.h>

#include "PrinterDriver.h"

// Constants
const char ATTR_NAME[] = "printer_settings";
const char PDF_COMPATIBILITY[] = "1.3";
const char PRINTER_SETTINGS_PATH[] = "printers/";
const int64 XRES = 360;
const int64 YRES = 360;
const int32 ORIENTATION = PrinterDriver::PORTRAIT_ORIENTATION;
const float RES = 72.0f;
const int32 PDF_COMPRESSION = 3;
const int32 UNITS = 1;
const float LETTER_W = 8.5f * RES;
const float LETTER_H = 11.0f * RES;
const BRect PAPER_RECT(0, 0, LETTER_W, LETTER_H);
const BRect PRINT_RECT(0, 0, LETTER_W, LETTER_H);
const bool  CREATE_WEB_LINKS  = false;
const float LINK_BORDER_WIDTH = 1.0;
const bool CREATE_BOOKMARKS = false;
const char BOOKMARK_DEFINITION_FILE[] = "";
const bool CREATE_XREFS = false;
const char XREFS_FILE[] = "";
const int32 CLOSE_OPTION = 0;
const char PDFLIB_LICENSE_KEY[] = "";
const char USER_PASSWORD[] = ""; // none if empty
const char MASTER_PASSWORD[] = ""; // none if empty
const char PERMISSIONS[] = ""; // all is allowed

/**
 * Class
 */
class PrinterSettings
{
private:
	BNode		fNode;
	status_t	fErr;

public:
	PrinterSettings() { }
	PrinterSettings(const char *printer); 
	PrinterSettings(BNode &printer);
	~PrinterSettings();

	status_t InitCheck();
	status_t WriteSettings(BMessage *msg);
	status_t ReadSettings(BMessage *msg);
	status_t GetDefaults(BMessage *msg);
	status_t Validate(const BMessage *msg);

	static void Update(BNode* node, BMessage* msg);
};


#endif

