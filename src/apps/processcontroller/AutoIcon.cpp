/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
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
		fBitmap = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32);

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
