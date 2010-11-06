/*
 * OpenSound media addon for BeOS and Haiku
 *
 * Copyright (c) 2007, François Revol (revol@free.fr)
 * Distributed under the terms of the MIT License.
 * 
 * Based on MultiAudio media addon
 * Copyright (c) 2002, 2003 Jerome Duval (jerome.duval@free.fr)
 */

#ifndef _OPENSOUNDADDON_H
#define _OPENSOUNDADDON_H

#include <MediaDefs.h>
#include <MediaFormats.h>
#include <MediaAddOn.h>

#define SETTINGS_FILE					"Media/oss_audio_settings"

class BEntry;

class OpenSoundAddOn :
	public BMediaAddOn
{
	public:
		virtual ~OpenSoundAddOn(void);
		explicit OpenSoundAddOn(image_id image);

		/**************************/
		/* begin from BMediaAddOn */
	public:
		virtual	status_t InitCheck(
			const char ** out_failure_text);
		virtual	int32 CountFlavors(void);
		virtual	status_t GetFlavorAt(
			int32 n,
			const flavor_info ** out_info);
		virtual	BMediaNode * InstantiateNodeFor(
			const flavor_info * info,
			BMessage * config,
			status_t * out_error);
		virtual	status_t GetConfigurationFor(
			BMediaNode * your_node,
			BMessage * into_message);
		virtual	bool WantsAutoStart(void);
		virtual	status_t AutoStart(
			int in_count,
			BMediaNode ** out_node,
			int32 * out_internal_id,
			bool * out_has_more);

		/* end from BMediaAddOn */
		/************************/

	private:
		status_t RecursiveScan(const char* path, BEntry *rootEntry = NULL);
		void SaveSettings(void);
		void LoadSettings(void);
		
		void RegisterMediaFormats(void);

		status_t 		fInitCheckStatus;
		BList			fDevices;

		BMessage fSettings;				// settings loaded from settings directory
};

extern "C" _EXPORT BMediaAddOn *make_media_addon(image_id you);

#endif /* _OPENSOUNDADDON_H */
