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
#include <MediaDefs.h>
#include <MediaAddOn.h>
#include <Errors.h>
#include <Node.h>
#include <Mime.h>
#include <StorageDefs.h>
#include <Path.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>

#include "MultiAudioNode.h"
#include "MultiAudioAddOn.h"
#include "MultiAudioDevice.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"

#define MULTI_SAVE

// instantiation function
extern "C" _EXPORT BMediaAddOn * make_media_addon(image_id image) {
	CALLED();
	return new MultiAudioAddOn(image);
}

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

MultiAudioAddOn::~MultiAudioAddOn()
{
	CALLED();
	
	void *device = NULL;
	for ( int32 i = 0; (device = fDevices.ItemAt(i)); i++ )
		delete device;
		
	SaveSettings();
}

MultiAudioAddOn::MultiAudioAddOn(image_id image) :
	BMediaAddOn(image),
	fDevices()
{
	CALLED();
	fInitCheckStatus = B_NO_INIT;
	
	if(RecursiveScan("/dev/audio/multi/")!=B_OK)
		return;
		
	LoadSettings();
	
	fInitCheckStatus = B_OK;
}

// -------------------------------------------------------- //
// BMediaAddOn impl
// -------------------------------------------------------- //

status_t MultiAudioAddOn::InitCheck(
	const char ** out_failure_text)
{
	CALLED();
	return B_OK;
}

int32 MultiAudioAddOn::CountFlavors()
{
	CALLED();
	return fDevices.CountItems();
}

status_t MultiAudioAddOn::GetFlavorAt(
	int32 n,
	const flavor_info ** out_info)
{
	CALLED();
	if (out_info == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // we refuse to crash because you were stupid
	}	
	if (n < 0 || n > fDevices.CountItems() - 1) {
		fprintf(stderr,"<- B_BAD_INDEX\n");
		return B_BAD_INDEX;
	}
		
	MultiAudioDevice *device = (MultiAudioDevice *) fDevices.ItemAt(n);
		
	flavor_info * infos = new flavor_info[1];
	MultiAudioNode::GetFlavor(&infos[0], n);
	infos[0].name = device->MD.friendly_name;
	(*out_info) = infos;
	return B_OK;
}

BMediaNode * MultiAudioAddOn::InstantiateNodeFor(
				const flavor_info * info,
				BMessage * config,
				status_t * out_error)
{
	CALLED();
	if (out_error == 0) {
		fprintf(stderr,"<- NULL\n");
		return 0; // we refuse to crash because you were stupid
	}
	
	MultiAudioDevice *device = (MultiAudioDevice*)fDevices.ItemAt(info->internal_id);
	if(device == NULL) {
		*out_error = B_ERROR;
		return NULL;
	}
	
#ifdef MULTI_SAVE
	if(fSettings.FindMessage(device->MD.friendly_name, config)==B_OK) {
		fSettings.RemoveData(device->MD.friendly_name);
	}
#endif
	
	MultiAudioNode * node
		= new MultiAudioNode(this,
						  device->MD.friendly_name,
						  device,
						  info->internal_id,
						  config);
	if (node == 0) {
		*out_error = B_NO_MEMORY;
		fprintf(stderr,"<- B_NO_MEMORY\n");
	} else { 
		*out_error = node->InitCheck();
	}
	return node;	
}

status_t
MultiAudioAddOn::GetConfigurationFor(BMediaNode * your_node, BMessage * into_message)
{
	CALLED();
#ifdef MULTI_SAVE
	if (into_message == 0) {
		into_message = new BMessage();
		MultiAudioNode * node = dynamic_cast<MultiAudioNode*>(your_node);
		if (node == 0) {
			fprintf(stderr,"<- B_BAD_TYPE\n");
			return B_BAD_TYPE;
		}
		if(node->GetConfigurationFor(into_message)==B_OK) {
			fSettings.AddMessage(your_node->Name(), into_message);
		}		
		return B_OK;
	}
#endif	
	// currently never called by the media kit. Seems it is not implemented.
	if (into_message == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // we refuse to crash because you were stupid
	}	
	MultiAudioNode * node = dynamic_cast<MultiAudioNode*>(your_node);
	if (node == 0) {
		fprintf(stderr,"<- B_BAD_TYPE\n");
		return B_BAD_TYPE;
	}
	return node->GetConfigurationFor(into_message);
}


bool MultiAudioAddOn::WantsAutoStart()
{
	CALLED();
	return false;
}

status_t MultiAudioAddOn::AutoStart(
				int in_count,
				BMediaNode ** out_node,
				int32 * out_internal_id,
				bool * out_has_more)
{
	CALLED();
	return B_OK;
}

status_t
MultiAudioAddOn::RecursiveScan(char* rootPath, BEntry *rootEntry = NULL)
{
	CALLED();
	
	BDirectory root;
	if(rootEntry!=NULL)
		root.SetTo(rootEntry);
	else if(rootPath!=NULL) {
		root.SetTo(rootPath);
	} else {
		PRINT(("Error in MultiAudioAddOn::RecursiveScan null params\n"));
		return B_ERROR;
	}
	
	BEntry entry;
	
	while(root.GetNextEntry(&entry) > B_ERROR) {

		if(entry.IsDirectory()) {
			RecursiveScan(rootPath, &entry);
		} else {
			BPath path;
			entry.GetPath(&path);
			MultiAudioDevice *device = new MultiAudioDevice(path.Path() + strlen(rootPath), path.Path());
			if (device) {
				if (device->InitCheck() == B_OK)
					fDevices.AddItem(device);
				else
					delete device;
			}
		}
	}
	
	return B_OK;
}


void
MultiAudioAddOn::SaveSettings(void)
{
	CALLED();
	BPath path;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(SETTINGS_FILE);
		BFile file(path.Path(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
		if(file.InitCheck()==B_OK)
			fSettings.Flatten(&file);
	}
}


void
MultiAudioAddOn::LoadSettings(void)
{
	CALLED();
	fSettings.MakeEmpty();
	
	BPath path;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(SETTINGS_FILE);
		BFile file(path.Path(),B_READ_ONLY);
		if((file.InitCheck()==B_OK)&&(fSettings.Unflatten(&file)==B_OK))
		{
			PRINT_OBJECT(fSettings);
		} else {
			PRINT(("Error unflattening settings file %s\n",path.Path()));
		}	
	}
}
