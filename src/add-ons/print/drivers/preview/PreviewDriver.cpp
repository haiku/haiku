/*
 * Copyright 2003-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#include "PreviewDriver.h"
#include "Preview.h"


#define PREVIEW_DRIVER_DEBUG 0


PreviewDriver::PreviewDriver(BNode* spoolDir)
	: PrinterDriver(spoolDir)
{
};


PreviewDriver::~PreviewDriver()
{
}


status_t
PreviewDriver::PrintJob(BFile *jobFile, BMessage *jobMsg)
{
#if PREVIEW_DRIVER_DEBUG
	return PrinterDriver::PrintJob(jobFile, jobMsg);
#else
	PreviewWindow* w = new PreviewWindow(jobFile);
	return w->Go();
#endif
}


PrinterDriver*
instanciate_driver(BNode *spoolDir)
{
	return new PreviewDriver(spoolDir);
}
