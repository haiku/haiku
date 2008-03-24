/*
 * Copyright 2001-2008, Haiku. All rights reserved.
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
#include <String.h>


#include "Driver.h"
#include "PrinterDriver.h"


BMessage*
take_job(BFile *spoolFile, BNode *spoolDir, BMessage *msg)
{
	PrinterDriver *driver = instanciate_driver(spoolDir);
	status_t status = driver->PrintJob(spoolFile, msg);
	delete driver;

	msg = new BMessage('okok');
	if (status != B_OK)
		msg->what = 'baad';

	return msg;
}


BMessage*
config_page(BNode *spoolDir, BMessage *msg)
{
	BString printerName;
	spoolDir->ReadAttrString("Printer Name", &printerName);

	BMessage *pagesetupMsg = new BMessage(*msg);
	pagesetupMsg->what = 'okok';

	PrinterDriver *driver = instanciate_driver(spoolDir);
	if (driver->PageSetup(pagesetupMsg, printerName.String()) != B_OK) {
		delete pagesetupMsg;
		pagesetupMsg = NULL;
	}
	delete driver;
	return pagesetupMsg;
}


BMessage*
config_job(BNode *spoolDir, BMessage *msg)
{
	BString printerName;
	spoolDir->ReadAttrString("Printer Name", &printerName);

	BMessage *jobsetupMsg = new BMessage(*msg);
	jobsetupMsg->what = 'okok';

	PrinterDriver *driver = instanciate_driver(spoolDir);
	if (driver->JobSetup(jobsetupMsg, printerName.String()) != B_OK) {
		delete jobsetupMsg;
		jobsetupMsg = NULL;
	}
	delete driver;
	return jobsetupMsg;
}


char*
add_printer(char *printerName)
{
	return printerName;
}


BMessage*
default_settings(BNode* spoolDir)
{
	PrinterDriver *driver = instanciate_driver(spoolDir);
	BMessage *settings = driver->GetDefaultSettings();
	delete driver;
	return settings;
}
