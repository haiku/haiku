/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VIDEO_ADDON_H
#define _VIDEO_ADDON_H

#include <media/MediaAddOn.h>

#define TOUCH(x) ((void)(x))

extern "C" _EXPORT BMediaAddOn *make_media_addon(image_id you);

class CamRoster;
class CamDevice;

class WebCamMediaAddOn : public BMediaAddOn {
public:
						WebCamMediaAddOn(image_id imid);
	virtual 			~WebCamMediaAddOn();

	virtual	status_t	InitCheck(const char **out_failure_text);

	virtual	int32		CountFlavors();
	virtual	status_t	GetFlavorAt(int32 n, const flavor_info ** out_info);
	virtual	BMediaNode	*InstantiateNodeFor(
							const flavor_info * info,
							BMessage * config,
							status_t * out_error);

	virtual	status_t	GetConfigurationFor(BMediaNode *node, BMessage *message)
								{ TOUCH(node); TOUCH(message); return B_OK; }
	virtual	status_t	SaveConfigInfo(BMediaNode *node, BMessage *message)
								{ TOUCH(node); TOUCH(message); return B_OK; }

	virtual	bool		WantsAutoStart() { return false; }
	virtual	status_t	AutoStart(int in_count, BMediaNode **out_node,
								int32 *out_internal_id, bool *out_has_more)
								{	TOUCH(in_count); TOUCH(out_node);
									TOUCH(out_internal_id); TOUCH(out_has_more);
									return B_ERROR; }
		// those are for use by CamDevices
		status_t		CameraAdded(CamDevice* device);
		status_t		CameraRemoved(CamDevice* device);
		void			FillDefaultFlavorInfo(flavor_info* info);

private:
	uint32				fInternalIDCounter;
	status_t			fInitStatus;
	flavor_info			fDefaultFlavorInfo;
	media_format		fMediaFormat;
	CamRoster*			fRoster;
};

#endif
