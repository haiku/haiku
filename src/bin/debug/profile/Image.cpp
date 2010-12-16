/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Image.h"


Image::Image(SharedImage* image, const image_info& info, team_id owner,
	int32 creationEvent)
	:
	fImage(image),
	fID(info.id),
	fOwner(owner),
	fLoadDelta((addr_t)info.text - (addr_t)image->Info().text),
	fCreationEvent(creationEvent),
	fDeletionEvent(-1)
{
	fImage->AcquireReference();
}


Image::~Image()
{
	fImage->ReleaseReference();
}
