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
const char kAttrPageSettings[] = "pdfwriter/page_settings";
const char kAttrJobSettings[] = "pdfwriter/job_settings";
// page settings
const char kPDFCompatibilty[] = "1.3";
const char kPrinterSettingsPath[] = "printers/";
const int64 kXRes = 360;
const int64 kYRes = 360;
const int32 kOrientation = PrinterDriver::PORTRAIT_ORIENTATION;
const float kRes = 72.0f;
const int32 kPDFCompression = 3;
const int32 kUnits = 1;
const float kLetterW = 8.5f * kRes;
const float kLetterH = 11.0f * kRes;
const BRect kPaperRect(0, 0, kLetterW, kLetterH);
const BRect kPrintRect(0, 0, kLetterW, kLetterH);
const bool  kCreateWebLinks  = false;
const float kLinkBorderWidth = 1.0;
const bool kCreateBookmarks = false;
const char kBookmarkDefinitionFile[] = "";
const bool kCreateXRefs = false;
const char kXRefsFile[] = "";
const int32 kCloseStatusWindow = 0;
// requires commercial version of PDFlib 
#if HAVE_FULLVERSION_PDF_LIB
const char kPDFLibLicenseKey[] = "";
const char kUserPassword[] = ""; // none if empty
const char kMasterPassword[] = ""; // none if empty
const char kPermissions[] = ""; // all is allowed
#endif
// job settings (we do not have default values for them)

/**
 * Class
 */
class PrinterSettings
{
public:
	enum Kind {
		kPageSettings,
		kJobSettings
	};

	PrinterSettings(BNode &printer, Kind kind);
	~PrinterSettings();

	status_t InitCheck();
	status_t WriteSettings(BMessage *msg);
	status_t ReadSettings(BMessage *msg);
	status_t GetDefaults(BMessage *msg);
	status_t Validate(const BMessage *msg);

	static void Read(BNode* node, BMessage* msg, Kind kind);
	static void Update(BNode* node, BMessage* msg, Kind kind);

private:
	PrinterSettings();
	const char* GetAttrName() const;

	BNode		fNode;
	status_t	fErr;
	enum Kind   fKind;
};


#endif

