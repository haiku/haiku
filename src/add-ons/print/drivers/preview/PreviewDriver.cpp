/*
 * Copyright 2003-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#include "PreviewDriver.h"

status_t PreviewDriver::PrintJob(BFile *jobFile, BMessage *jobMsg) {
	PreviewWindow* w;
	w = new PreviewWindow(jobFile);
	return w->Go();
}

PrinterDriver* instanciate_driver(BNode *spoolDir)
{
	return new PreviewDriver(spoolDir);
}

// About dialog text:
const char* 
kAbout =
"Preview for Haiku\n"
"© 2003-2007 Haiku\n"
"by Michael Pfeiffer\n"
"\n"
"Based on PDF Writer by\nPhilippe Houdoin, Simon Gauvin, Michael Pfeiffer\n"
;


