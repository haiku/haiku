/*
* Copyright (C) 2009-2010 David McPaul
*
* All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef _VIDEO_MIXER_ADD_ON_H
#define _VIDEO_MIXER_ADD_ON_H


#include <MediaAddOn.h>


class VideoMixerAddOn : public BMediaAddOn {
public:
	virtual					~VideoMixerAddOn(void);
	explicit				VideoMixerAddOn(image_id image);

	virtual	status_t		InitCheck(const char **out_failure_text);
	virtual	int32			CountFlavors(void);
	virtual	status_t		GetFlavorAt(int32 n,
								const flavor_info **out_info);
	virtual	BMediaNode*		InstantiateNodeFor(const flavor_info *info,
								BMessage *config, status_t *out_error);
	virtual	status_t 		GetConfigurationFor(BMediaNode *your_node,
								BMessage *into_message);
	virtual	bool 			WantsAutoStart(void);
	virtual	status_t 		AutoStart(int in_count,	BMediaNode **out_node,
								int32 *out_internal_id,
								bool *out_has_more);

private:
	uint32					refCount;
};

extern "C" _EXPORT BMediaAddOn *make_video_mixer_add_on(image_id you);

#endif /* _VIDEO_MIXER_ADD_ON_H */
