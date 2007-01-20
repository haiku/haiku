/*
 * Copyright 2001-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin	
 *		Michael Pfeiffer
 */


#include <stdio.h>
#include <string.h>

#include <StorageKit.h>

#include "Driver.h"
#include "PrinterDriver.h"

// --------------------------------------------------
BMessage* 
take_job(BFile *spoolFile, BNode *spoolDir, BMessage *msg) 
{
	PrinterDriver *driver;
	
	driver = instanciate_driver(spoolDir);
	if (driver->PrintJob(spoolFile, msg) == B_OK) {
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
	BMessage		*pagesetupMsg = new BMessage(*msg);
	PrinterDriver	*driver;
	const char		*printerName;
	char			buffer[B_ATTR_NAME_LENGTH+1];

	// retrieve the printer (spool) name.
	printerName = NULL;
	if (spoolDir->ReadAttr("Printer Name", B_STRING_TYPE, 1, buffer, B_ATTR_NAME_LENGTH+1) > 0) {
		printerName = buffer;
	}

	driver = instanciate_driver(spoolDir);
	if (driver->PageSetup(pagesetupMsg, printerName) == B_OK) {
		pagesetupMsg->what = 'okok';
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
	BMessage		*jobsetupMsg = new BMessage(*msg);
	PrinterDriver	*driver;
	const char		*printerName;
	char			buffer[B_ATTR_NAME_LENGTH+1];

	// retrieve the printer (spool) name.
	printerName = NULL;
	if (spoolDir->ReadAttr("Printer Name", B_STRING_TYPE, 1, buffer, B_ATTR_NAME_LENGTH+1) > 0) {
		printerName = buffer;
	}
	driver = instanciate_driver(spoolDir);
	if (driver->JobSetup(jobsetupMsg, printerName) == B_OK) {
		jobsetupMsg->what = 'okok';
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
	status_t st;
	PrinterDriver* driver;
	driver = instanciate_driver(NULL);
	st = driver->PrinterSetup(printerName);
	delete driver;
	if (st == B_OK) {
		return printerName; 
	} else {
		return NULL;
	}
}

/**
 * default_settings  
 *
 * @param BNode* printer spool directory
 * @return BMessage* the settings
 */
BMessage* 
default_settings(BNode* spoolDir)
{
	PrinterDriver* driver;
	BMessage* settings;
	driver = instanciate_driver(spoolDir);
	settings = driver->GetDefaultSettings();
	delete driver;
	return settings;
}
