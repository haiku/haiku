/*
 *  Printers Preference Application.
 *  Copyright (C) 2002 OpenBeOS. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "SpoolFolder.h"
#include "JobListView.h"
#include "pr_server.h"

#include "Messages.h"
#include "Globals.h"
#include "Jobs.h"
#include "PrintersWindow.h"

#include <Messenger.h>
#include <Bitmap.h>
#include <String.h>
#include <Alert.h>
#include <Mime.h>
#include <Roster.h>

SpoolFolder::SpoolFolder(PrintersWindow* window, PrinterItem* item, const BDirectory& spoolDir)
	: Folder(NULL, window, spoolDir)
	, fWindow(window)
	, fItem(item)
{
}

void SpoolFolder::Notify(Job* job, int kind) 
{
	switch (kind) {
		case kJobAdded: fWindow->AddJob(this, job);
			break;
		case kJobRemoved: fWindow->RemoveJob(this, job);
			break;
		case kJobAttrChanged: fWindow->UpdateJob(this, job);
			break;
		default: ;
	}
}
