/*
 * Copyright 2002-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#include "Preview.h"
#include "PrinterDriver.h"

class PreviewDriver : public PrinterDriver {
public:
	PreviewDriver(BNode* spoolDir) : PrinterDriver(spoolDir) {};
	~PreviewDriver() {};
	virtual status_t 		PrintJob(BFile *jobFile, BMessage *jobMsg);	
};
