/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include <support/Autolock.h>
#include <media/MediaFormats.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "AddOn.h"
#include "Producer.h"
#include "CamRoster.h"
#include "CamDebug.h"
#include "CamDevice.h"


WebCamMediaAddOn::WebCamMediaAddOn(image_id imid)
	: BMediaAddOn(imid),
	fInitStatus(B_NO_INIT),
	fRoster(NULL)
{
	PRINT((CH "()" CT));
	fInternalIDCounter = 0;
	/* Customize these parameters to match those of your node */
	fMediaFormat.type = B_MEDIA_RAW_VIDEO;
	fMediaFormat.u.raw_video = media_raw_video_format::wildcard;
	fMediaFormat.u.raw_video.interlace = 1;
	fMediaFormat.u.raw_video.display.format = B_RGB32;
	FillDefaultFlavorInfo(&fDefaultFlavorInfo);

	fRoster = new CamRoster(this);
	fRoster->Start();
//	if( fRoster->CountCameras() < 1 )
/*		fInitStatus = B_ERROR;
	else
*/
		fInitStatus = B_OK;
}


WebCamMediaAddOn::~WebCamMediaAddOn()
{
	fRoster->Stop();
	delete fRoster;
}


status_t
WebCamMediaAddOn::InitCheck(const char **out_failure_text)
{
	if (fInitStatus < B_OK) {
		*out_failure_text = "No cameras attached";
		return fInitStatus;
	}

	return B_OK;
}


int32
WebCamMediaAddOn::CountFlavors()
{
	PRINT((CH "()" CT));
	int32 count;
	if (!fRoster)
		return B_NO_INIT;
	if (fInitStatus < B_OK)
		return fInitStatus;

	/* This addon only supports a single flavor, as defined in the
	 * constructor */
	count = fRoster->CountCameras();
	return count;//(count > 0)?count:1;//1;
}


/*
 * The pointer to the flavor received only needs to be valid between
 * successive calls to BMediaAddOn::GetFlavorAt().
 */
status_t
WebCamMediaAddOn::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	PRINT((CH "(%d, ) roster %p is %" B_PRIx32 CT, n, fRoster, fInitStatus));
	int32 count;
	CamDevice* cam;
	if (!fRoster)
		return B_NO_INIT;
	if (fInitStatus < B_OK)
		return fInitStatus;

	count = fRoster->CountCameras();
	PRINT((CH ": %d cameras" CT, count));
	if (n >= count)//(n != 0)
		return B_BAD_INDEX;

	fRoster->Lock();
	cam = fRoster->CameraAt(n);
	*out_info = &fDefaultFlavorInfo;
	if (cam && cam->FlavorInfo())
		*out_info = cam->FlavorInfo();
	fRoster->Unlock();
	PRINT((CH ": returning flavor for %d, internal_id %d" CT, n, (*out_info)->internal_id));
	return B_OK;
}


BMediaNode *
WebCamMediaAddOn::InstantiateNodeFor(
		const flavor_info *info, BMessage* /*_config*/, status_t* /*_out_error*/)
{
	PRINT((CH "()" CT));
	VideoProducer *node;
	CamDevice *cam=NULL;

	if (fInitStatus < B_OK)
		return NULL;

	fRoster->Lock();
	for (uint32 i = 0; i < fRoster->CountCameras(); i++) {
		CamDevice *c;
		c = fRoster->CameraAt(i);
		PRINT((CH ": cam[%d]: %d, %s" CT, i, c->FlavorInfo()->internal_id, c->BrandName()));
		if (c && (c->FlavorInfo()->internal_id == info->internal_id)) {
			cam = c;
			break;
		}
	}
	fRoster->Unlock();
	if (!cam)
		return NULL;

#if 0
	fRoster->Lock();
	cam = fRoster->CameraAt(n);
	*out_info = &fDefaultFlavorInfo;
	if (cam && cam->FlavorInfo())
		*out_info = cam->FlavorInfo();
	fRoster->Unlock();
#endif
	/* At most one instance of the node should be instantiated at any given
	 * time. The locking for this restriction may be found in the VideoProducer
	 * class. */
	node = new VideoProducer(this, cam, cam->FlavorInfo()->name, cam->FlavorInfo()->internal_id);
	if (node && (node->InitCheck() < B_OK)) {
		delete node;
		node = NULL;
	}

	return node;
}


status_t
WebCamMediaAddOn::CameraAdded(CamDevice* device)
{
	PRINT((CH "()" CT));
	NotifyFlavorChange();
	return B_OK;
}


status_t
WebCamMediaAddOn::CameraRemoved(CamDevice* device)
{
	PRINT((CH "()" CT));
	NotifyFlavorChange();
	return B_OK;
}


void
WebCamMediaAddOn::FillDefaultFlavorInfo(flavor_info* info)
{
	info->name = "USB Web Camera";
	info->info = "USB Web Camera";
	info->kinds = B_BUFFER_PRODUCER | B_CONTROLLABLE | B_PHYSICAL_INPUT;
	info->flavor_flags = 0;//B_FLAVOR_IS_GLOBAL;
	info->internal_id = atomic_add((int32*)&fInternalIDCounter, 1);
	info->possible_count = 1;//0;
	info->in_format_count = 0;
	info->in_format_flags = 0;
	info->in_formats = NULL;
	info->out_format_count = 1;
	info->out_format_flags = 0;
	info->out_formats = &fMediaFormat;
}


BMediaAddOn *
make_media_addon(image_id id)
{
	return new WebCamMediaAddOn(id);
}
