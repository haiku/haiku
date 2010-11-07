/*
 * Copyright 2001-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema
 *		Michael Pfeiffer
 */
#ifndef PRINTER_DRIVER_ADD_ON_H
#define PRINTER_DRIVER_ADD_ON_H


#include <Directory.h>
#include <image.h>
#include <Message.h>
#include <Path.h>
#include <SupportDefs.h>


class PrinterDriverAddOn
{
public:
						PrinterDriverAddOn(const char* driver);
						~PrinterDriverAddOn();

			status_t	AddPrinter(const char* spoolFolderName);
			status_t	ConfigPage(BDirectory* spoolFolder,
							BMessage* settings);
			status_t	ConfigJob(BDirectory* spoolFolder,
							BMessage* settings);
			status_t	DefaultSettings(BDirectory* spoolFolder,
							BMessage* settings);
			status_t	TakeJob(const char* spoolFile,
							BDirectory* spoolFolder);

	static	status_t	FindPathToDriver(const char* driver, BPath* path);

private:
			bool		IsLoaded() const;
			status_t	CopyValidSettings(BMessage* settings,
							BMessage* newSettings);

	image_id	fAddOnID;
};


#endif
