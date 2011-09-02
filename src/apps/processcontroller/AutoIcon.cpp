/*
	ProcessController © 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org 

	This library is free software; you can redistribute it and/or 
	modify it under the terms of the GNU Lesser General Public 
	License as published by the Free Software Foundation; either 
	version 2.1 of the License, or (at your option) any later version. 

	This library is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	Lesser General Public License for more details. 

	You should have received a copy of the GNU Lesser General Public 
	License along with this library; if not, write to the Free Software 
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	
*/


#include "AutoIcon.h"
#include "Utilities.h"

#include <Bitmap.h>
#include <Entry.h>
#include <MimeType.h>
#include <NodeInfo.h>
#include <Roster.h>


AutoIcon::~AutoIcon()
{
	delete fBitmap;
}


BBitmap*
AutoIcon::Bitmap()
{
	if (fBitmap == NULL) {
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		fBitmap = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32);
#else
		fBitmap = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);
#endif
		if (fSignature) {
			entry_ref ref;
			be_roster->FindApp (fSignature, &ref);
			if (BNodeInfo::GetTrackerIcon(&ref, fBitmap, B_MINI_ICON) != B_OK) {
				BMimeType genericAppType(B_APP_MIME_TYPE);
				genericAppType.GetIcon(fBitmap, B_MINI_ICON);
			}
		}

		if (fbits)
			fBitmap->SetBits(fbits, 256, 0, B_CMAP8);
	}
	return fBitmap;
}
