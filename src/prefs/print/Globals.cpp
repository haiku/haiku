/*
 *  Printers Preference Application.
 *  Copyright (C) 2001, 2002 OpenBeOS. All Rights Reserved.
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

#include "Globals.h"
#include "pr_server.h"

#include <Roster.h>

#include <stdio.h>

status_t GetPrinterServerMessenger(BMessenger& msgr)
{
		// If print server is not yet running, start it
	if (!be_roster->IsRunning(PSRV_SIGNATURE_TYPE)) {
		be_roster->Launch(PSRV_SIGNATURE_TYPE);
	}
	
	msgr = BMessenger(PSRV_SIGNATURE_TYPE);

	return msgr.IsValid() ? B_OK : B_ERROR;
}
