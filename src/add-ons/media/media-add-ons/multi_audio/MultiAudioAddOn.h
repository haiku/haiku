/*
 * Copyright (c) 2002, Jerome Duval (jerome.duval@free.fr)
 * Distributed under the terms of the MIT License.
 */
#ifndef MULTI_AUDIO_ADDON_H
#define MULTI_AUDIO_ADDON_H


#include <MediaDefs.h>
#include <MediaAddOn.h>
#include <Message.h>

class BEntry;


class MultiAudioAddOn : public BMediaAddOn {
public:
						MultiAudioAddOn(image_id image);
	virtual				~MultiAudioAddOn();

	virtual	status_t	InitCheck(const char** _failureText);
	virtual	int32		CountFlavors();
	virtual	status_t	GetFlavorAt(int32 i, const flavor_info** _info);
	virtual	BMediaNode*	InstantiateNodeFor(const flavor_info* info,
							BMessage* config, status_t* _error);
	virtual	status_t	GetConfigurationFor(BMediaNode* node,
							BMessage* message);
	virtual	bool		WantsAutoStart();
	virtual	status_t	AutoStart(int count, BMediaNode** _node,
							int32* _internalID, bool* _hasMore);

private:
			status_t	_RecursiveScan(const char* path, BEntry* rootEntry = NULL,
							uint32 depth = 0);
			void		_SaveSettings();
			void		_LoadSettings();

private:
	status_t 		fInitStatus;
	BList			fDevices;

	BMessage		fSettings;
		// loaded from settings directory
};

extern "C" BMediaAddOn* make_media_addon(image_id you);

#endif	// MULTI_AUDIO_ADDON_H
