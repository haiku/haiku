/*
 * Copyright 2002-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#include "PrinterDriver.h"


class PreviewDriver : public PrinterDriver {
public:
						PreviewDriver(BNode* spoolDir);
						~PreviewDriver();

	virtual status_t	PrintJob(BFile *jobFile, BMessage *jobMsg);
};
