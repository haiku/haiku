/*
 * multiaudio replacement media addon for BeOS
 *
 * Copyright (c) 2002, Jerome Duval (jerome.duval@free.fr)
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef _MULTIAUDIOADDON_H
#define _MULTIAUDIOADDON_H

#include <MediaDefs.h>
#include <MediaAddOn.h>

class MultiAudioAddOn :
    public BMediaAddOn
{
public:
	virtual ~MultiAudioAddOn(void);
	explicit MultiAudioAddOn(image_id image);

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
	status_t RecursiveScan(char* path, BEntry *rootEntry = NULL);
	
	status_t 		fInitCheckStatus;
	BList			fDevices;
};

extern "C" _EXPORT BMediaAddOn *make_media_addon( image_id you );

#endif /* _MULTIAUDIOADDON_H */
