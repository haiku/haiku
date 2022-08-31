/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "AutoIcon.h"
#include "Utilities.h"

#include <Bitmap.h>
#include <ControlLook.h>
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
	if (fBitmap != NULL)
		return fBitmap;

	if (fSignature) {
		fBitmap = new BBitmap(BRect(BPoint(0, 0),
			be_control_look->ComposeIconSize(B_MINI_ICON)), B_RGBA32);

		entry_ref ref;
		be_roster->FindApp (fSignature, &ref);
		if (BNodeInfo::GetTrackerIcon(&ref, fBitmap, (icon_size)-1) != B_OK) {
			BMimeType genericAppType(B_APP_MIME_TYPE);
			genericAppType.GetIcon(fBitmap, (icon_size)(fBitmap->Bounds().IntegerWidth() + 1));
		}
	} else if (fbits) {
		fBitmap = new BBitmap(BRect(BPoint(0, 0),
			BSize(B_MINI_ICON - 1, B_MINI_ICON - 1)), B_RGBA32);

		fBitmap->SetBits(fbits, 256, 0, B_CMAP8);
	}
	return fBitmap;
}
