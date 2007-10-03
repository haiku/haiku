/*
 * Copyright 2002 David Shipman,
 * Copyright 2003-2007 Marcus Overhagen
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AUDIOMIXER_ADDON_H
#define _AUDIOMIXER_ADDON_H

#include <MediaAddOn.h>

class AudioMixerAddon : public BMediaAddOn {
	public:
		virtual 					~AudioMixerAddon();
		explicit 					AudioMixerAddon(image_id image);
	
		virtual	status_t			InitCheck(const char** out_failure_text);
		virtual	int32				CountFlavors();
		virtual	status_t 			GetFlavorAt(int32 n,
										const flavor_info ** out_info);
		virtual	BMediaNode *		InstantiateNodeFor(
										const flavor_info *info,
										BMessage *config,
										status_t *out_error);
		virtual	status_t			GetConfigurationFor(
										BMediaNode *your_node,
										BMessage *into_message);

		virtual	bool				WantsAutoStart();
		virtual	status_t			AutoStart(int in_index,
										BMediaNode **out_node,
										int32 *out_internal_id,
										bool *out_has_more);

	private:
		media_format 				*fFormat;
		flavor_info					*fInfo;
};
#endif
