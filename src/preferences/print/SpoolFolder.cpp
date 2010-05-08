/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */


#include "SpoolFolder.h"

#include "Jobs.h"
#include "PrintersWindow.h"
//#include "pr_server.h"


SpoolFolder::SpoolFolder(PrintersWindow* window, PrinterItem* item, 
	const BDirectory& spoolDir)
	: 
	Folder(NULL, window, spoolDir),
	fWindow(window),
	fItem(item)
{
}


void
SpoolFolder::Notify(Job* job, int kind)
{
	switch (kind) {
		case kJobAdded:
			fWindow->AddJob(this, job);
			break;
		case kJobRemoved:
			fWindow->RemoveJob(this, job);
			break;
		case kJobAttrChanged:
			fWindow->UpdateJob(this, job);
			break;
		default:
			break;
	}
}
