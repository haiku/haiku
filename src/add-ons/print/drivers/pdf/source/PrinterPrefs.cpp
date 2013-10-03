/*

PrinterPrefs.cpp

Copyright (c) 2002 OpenBeOS. 

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

#ifndef PRINTER_PREFS_H
#include "PrinterPrefs.h"
#endif

/**
 * Constructor
 *
 * @param none
 * @return none
 */
PrinterPrefs::PrinterPrefs() 
{
}

/**
 * Destructor
 *
 * @param none
 * @return none
 */
PrinterPrefs::~PrinterPrefs() 
{
}

/**
 * Init the settings message from settings file
 *
 * @param BMessage*, the settings
 * @return status_t, any result from the operation
 */
status_t 
PrinterPrefs::LoadSettings(BMessage *settings)
{
	status_t status;
	BFile file;

	status = find_directory(B_SYSTEM_SETTINGS_DIRECTORY, &fSettingsPath);
	if (status != B_OK) {
		// Could not find settings folder.
		return status;
	}

	fSettingsPath.Append(SETTINGS_FILE_NAME);
	status = file.SetTo(fSettingsPath.Path(), B_READ_ONLY);
	if (status == B_OK) {
		status = settings->Unflatten(&file);
	}	

	return status;
}

/**
 * Save the settings
 *
 * @param BMessage*, the settings
 * @return status_t 
 */
status_t 
PrinterPrefs::SaveSettings(BMessage* settings)
{
	status_t status;
	BFile file;

	status = file.SetTo(fSettingsPath.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (status == B_OK) {
		return settings->Flatten(&file);
	}
	return status;
}


