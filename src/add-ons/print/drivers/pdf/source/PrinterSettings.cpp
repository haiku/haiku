/*

Printer Settings helper class

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
#include "PrinterSettings.h"
#endif

#include <fs_attr.h>

#include "PrinterPrefs.h"
#include "Utils.h"

/**
 * Constructor
 *
 * @param BNode& the spooler folder
 * @return void
 */
PrinterSettings::PrinterSettings(BNode &printer, Kind kind)
{
	fNode = printer;	
	fErr = fNode.InitCheck();
	fKind = kind;
}

/**
 * InitCheck, check status of initialization
 *
 * @param none
 * @return status_t the status
 */
status_t 
PrinterSettings::InitCheck()
{
	return fErr;
}

/**
 * Destructor
 *
 * @param none
 * @return void
 */
PrinterSettings::~PrinterSettings() { }

/**
 * ReadSettings() return the settings for the printer from flattened BMessage
 * 	attached as an attribute to the spool folder node.
 *
 * @param BMessage* the settings in a message object
 * @return status_t the status of the read operation
 */
status_t 
PrinterSettings::ReadSettings(BMessage *msg)
{
	attr_info info;
	char *data = NULL;
	status_t err = B_ERROR;

	msg->MakeEmpty();
	msg->what = 0;
	
	if (fNode.GetAttrInfo(GetAttrName(), &info) == B_OK) {
		data = (char *)malloc(info.size);
		if (data != NULL) { 
			err = fNode.ReadAttr(GetAttrName(), B_MESSAGE_TYPE, 0, data, info.size); 
			if (err >= 0) {
				err = B_OK;
				msg->Unflatten(data);	 
			}
			free(data);
		}
	} else { // no attributes found, then we create them
		GetDefaults(msg);
		WriteSettings(msg);
		err = B_OK;
	}
	return err;
}

/**
 * WriteSettings() save the settings in a flattened BMessage and attach this 
 * 	to the node as an attribute
 *
 * @param BMessage* the settings to save in a BMessage object
 * @return status_t the status of the write operation
 */
status_t 
PrinterSettings::WriteSettings(BMessage* msg)
{
	size_t length;	
 	char *data = NULL;
	status_t err = B_ERROR;
	
	length = msg->FlattenedSize();
	data = (char *) malloc(length);
	if (data != NULL) {
		msg->Flatten(data, length);
		err = fNode.WriteAttr(GetAttrName(), B_MESSAGE_TYPE, 0, data, length);
		if (err > 0) {
			err = B_OK;
		}
		free(data);
	}
	return err;
}

/**
 * GetDefaults() return the default settings for the printer using:
 *		1) text file pdf_printer_settings in /home/config/settings
 * 		2) if file not found return hard coded in this method... Yuck!
 *
 * @param BMessage* the settings in a message object
 * @return status_t the status of the read operation
 */
status_t 
PrinterSettings::GetDefaults(BMessage *msg)
{
	// check to see if there is a pdf_printer_settings file
	PrinterPrefs prefs;
	BMessage settings;
	settings.what = 'okok';
	
	if (prefs.LoadSettings(&settings) == B_OK && Validate(&settings) == B_OK) {
		// yes, copy the settings into message
		*msg = settings;
	} else {
		// set default value if property not set
		msg->AddInt64("xres", kXRes);
		msg->AddInt64("yres", kYRes);
		msg->AddInt32("orientation", kOrientation);
		msg->AddRect("paper_rect", kPaperRect);
		msg->AddRect("printable_rect", kPrintRect);
		msg->AddFloat("scaling", kRes);
		msg->AddString("pdf_compatibility", kPDFCompatibilty);
		msg->AddInt32("pdf_compression", kPDFCompression);
		msg->AddInt32("units", kUnits);
		msg->AddBool("create_web_links", kCreateWebLinks);
		msg->AddFloat("link_border_width", kLinkBorderWidth);
		msg->AddBool("create_bookmarks", kCreateBookmarks);
		msg->AddString("bookmark_definition_file", kBookmarkDefinitionFile);
		msg->AddBool("create_xrefs", kCreateXRefs);
		msg->AddString("xrefs_file", kXRefsFile);
		msg->AddInt32("close_option", kCloseStatusWindow);
#if HAVE_FULLVERSION_PDF_LIB
		msg->AddString("pdflib_license_key", kPDFLibLicenseKey);
		msg->AddString("master_password", kMasterPassword);
		msg->AddString("user_password", kUserPassword);
		msg->AddString("permissions", kPermissions);
#endif
		
		// create pdf_printer_settings file
		prefs.SaveSettings(msg);
	}

	return B_OK;
}

/**
 * Validate(), check that the message is built correctly for the PDF driver
 			   Safe: does not modify the message
 *
 * @param BMessage*, the message to validate
 * @return status_t, B_OK if the message is correctly structured
 *					 B_ERROR if the message is corrupt
 */
status_t 
PrinterSettings::Validate(const BMessage *msg)
{
	int64 i64;
	int32 i32;
	float f;
	BRect r;
	BString s;
	bool b;

	// Test for data field existance
	if (msg->FindInt64("xres", 0, &i64) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindInt64("yres", 0, &i64) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindInt32("orientation", &i32) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindRect("paper_rect", &r) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindRect("printable_rect", &r) != B_OK) { 
		return B_ERROR;
	}
	if (msg->FindFloat("scaling", &f)  != B_OK) {
		return B_ERROR;
	}
	if (msg->FindString("pdf_compatibility", &s) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindInt32("pdf_compression", &i32) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindInt32("units", &i32) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindBool("create_web_links", &b) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindFloat("link_border_width", &f) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindBool("create_bookmarks", &b) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindString("bookmark_definition_file", &s) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindBool("create_xrefs", &b) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindString("xrefs_file", &s) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindInt32("close_option", &i32) != B_OK) {
		return B_ERROR;
	}
#if HAVE_FULLVERSION_PDF_LIB
	if (msg->FindString("pdflib_license_key", &s) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindString("user_password", &s) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindString("master_password", &s) != B_OK) {
		return B_ERROR;
	}
	if (msg->FindString("permissions", &s) != B_OK) {
		return B_ERROR;
	}
#endif
	// message ok
	return B_OK;
}


void
PrinterSettings::Read(BNode* node, BMessage* msg, Kind kind)
{
	PrinterSettings ps(*node, kind);
	if (ps.Validate(msg) != B_OK) {
		msg->MakeEmpty();
	}
	BMessage settings;
	// check for previously saved settings
	if (ps.ReadSettings(&settings) != B_OK || ps.Validate(&settings) != B_OK) {
		// if there were none, then create a default set...
		ps.GetDefaults(&settings);
	}
	AddFields(msg, &settings, false);
}

void
PrinterSettings::Update(BNode* node, BMessage* msg, Kind kind)
{
	// Validate the message so that it is good for PDF Writer GUI 
	PrinterSettings ps(*node, kind);
	ps.WriteSettings(msg);
}

const char*
PrinterSettings::GetAttrName() const
{
	if (fKind == kJobSettings) {
		return kAttrJobSettings;
	} else {
		return kAttrPageSettings;
	}
}
