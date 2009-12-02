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

	virtual	status_t			InitCheck(const char** _failureText);
	virtual	int32				CountFlavors();
	virtual	status_t 			GetFlavorAt(int32 n,
									const flavor_info** _info);
	virtual	BMediaNode *		InstantiateNodeFor(const flavor_info* info,
									BMessage* config, status_t* _error);
	virtual	status_t			GetConfigurationFor(BMediaNode* node,
									BMessage* toMmessage);

	virtual	bool				WantsAutoStart();
	virtual	status_t			AutoStart(int index, BMediaNode** _node,
									int32* _internalID, bool* _hasMore);

private:
			media_format*		fFormat;
			flavor_info*		fInfo;
};

#endif	// _AUDIOMIXER_ADDON_H
