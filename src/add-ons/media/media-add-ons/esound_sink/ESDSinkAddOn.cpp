/*
 * ESounD media addon for BeOS
 *
 * Copyright (c) 2006 François Revol (revol@free.fr)
 * 
 * Based on Multi Audio addon for Haiku,
 * Copyright (c) 2002, 2003 Jerome Duval (jerome.duval@free.fr)
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
#define _ZETA_TS_FIND_DIR_ 1
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

#include "ESDSinkNode.h"
#include "ESDSinkAddOn.h"
#include "ESDEndpoint.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
//#undef DEBUG
//#define DEBUG 4
#include "debug.h"
#include <Debug.h>

//#define MULTI_SAVE

// instantiation function
extern "C" _EXPORT BMediaAddOn * make_media_addon(image_id image) {
	CALLED();
	return new ESDSinkAddOn(image);
}

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

ESDSinkAddOn::~ESDSinkAddOn()
{
	CALLED();
	
	void *device = NULL;
	for ( int32 i = 0; (device = fDevices.ItemAt(i)); i++ )
		delete (ESDEndpoint *)device;
		
	SaveSettings();
}

ESDSinkAddOn::ESDSinkAddOn(image_id image) :
	BMediaAddOn(image),
	fDevices()
{
	CALLED();
	fInitCheckStatus = B_NO_INIT;
	
	LoadSettings();
	
	if(SetupDefaultSinks()!=B_OK)
		return;
		
	fInitCheckStatus = B_OK;
}

// -------------------------------------------------------- //
// BMediaAddOn impl
// -------------------------------------------------------- //

status_t ESDSinkAddOn::InitCheck(
	const char ** out_failure_text)
{
	CALLED();
	return B_OK;
}

int32 ESDSinkAddOn::CountFlavors()
{
	CALLED();
	//return fDevices.CountItems();
	return 1;
}

status_t ESDSinkAddOn::GetFlavorAt(
	int32 n,
	const flavor_info ** out_info)
{
	CALLED();
	//if (n < 0 || n > fDevices.CountItems() - 1) {
	if (n < 0 || n > 1) {
		fprintf(stderr,"<- B_BAD_INDEX\n");
		return B_BAD_INDEX;
	}
		
	//ESDEndpoint *device = (ESDEndpoint *) fDevices.ItemAt(n);
		
	flavor_info * infos = new flavor_info[1];
	ESDSinkNode::GetFlavor(&infos[0], n);
//	infos[0].name = device->MD.friendly_name;
	(*out_info) = infos;
	return B_OK;
}

BMediaNode * ESDSinkAddOn::InstantiateNodeFor(
				const flavor_info * info,
				BMessage * config,
				status_t * out_error)
{
	CALLED();
		
	BString name = "ESounD Sink";
#ifdef MULTI_SAVE
	ESDEndpoint *device = (ESDEndpoint *) fDevices.ItemAt(info->internal_id);
	if (device)
		device->GetFriendlyName(name);
	if(fSettings.FindMessage(name.String(), config)==B_OK) {
		fSettings.RemoveData(name.String());
	}
#endif
	
	
	ESDSinkNode * node
		= new ESDSinkNode(this,
						  (char *)name.String(),
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
ESDSinkAddOn::GetConfigurationFor(BMediaNode * your_node, BMessage * into_message)
{
	CALLED();
#ifdef MULTI_SAVE
		if (!into_message)
			into_message = new BMessage();
		ESDSinkNode * node = dynamic_cast<ESDSinkNode*>(your_node);
		if (node == 0) {
			fprintf(stderr,"<- B_BAD_TYPE\n");
			return B_BAD_TYPE;
		}
		if(node->GetConfigurationFor(into_message)==B_OK) {
			fSettings.AddMessage(your_node->Name(), into_message);
		}		
		return B_OK;
#endif	
	// currently never called by the media kit. Seems it is not implemented.
#if 0
	ESDSinkNode * node = dynamic_cast<ESDSinkNode*>(your_node);
	if (node == 0) {
		fprintf(stderr,"<- B_BAD_TYPE\n");
		return B_BAD_TYPE;
	}
	return node->GetConfigurationFor(into_message);
#endif
	return B_ERROR;
}

#if 0
bool ESDSinkAddOn::WantsAutoStart()
{
	CALLED();
	return true;//false;
}

status_t ESDSinkAddOn::AutoStart(
				int in_count,
				BMediaNode ** out_node,
				int32 * out_internal_id,
				bool * out_has_more)
{
	CALLED();
	const flavor_info *fi;
	status_t err;
	
	// XXX: LEAK!
	PRINT(("AutoStart: in_count=%d\n", in_count));
//	if (in_count < 1)
//		return EINVAL;
	*out_internal_id = 0;
	*out_has_more = false;
	err = GetFlavorAt(0, (const flavor_info **)&fi);
	if (err < 0)
		return err;
	*out_node = InstantiateNodeFor((const flavor_info *)fi, NULL, &err);
	delete fi;
	if (err < 0) 
		return err;
	return B_OK+1;
}
#endif

status_t
ESDSinkAddOn::SetupDefaultSinks()
{
	CALLED();
#if 0	
	BDirectory root;
	if(rootEntry!=NULL)
		root.SetTo(rootEntry);
	else if(rootPath!=NULL) {
		root.SetTo(rootPath);
	} else {
		PRINT(("Error in ESDSinkAddOn::RecursiveScan null params\n"));
		return B_ERROR;
	}
	
	BEntry entry;
	
	while(root.GetNextEntry(&entry) > B_ERROR) {

		if(entry.IsDirectory()) {
			RecursiveScan(rootPath, &entry);
		} else {
			BPath path;
			entry.GetPath(&path);
			ESDEndpoint *device = new ESDEndpoint(path.Path() + strlen(rootPath), path.Path());
			if (device) {
				if (device->InitCheck() == B_OK)
					fDevices.AddItem(device);
				else
					delete device;
			}
		}
	}
	
#endif
	return B_OK;
}


void
ESDSinkAddOn::SaveSettings(void)
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
ESDSinkAddOn::LoadSettings(void)
{
	CALLED();
	fSettings.MakeEmpty();
	
	BPath path;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(SETTINGS_FILE);
		BFile file(path.Path(),B_READ_ONLY);
		if((file.InitCheck()==B_OK)&&(fSettings.Unflatten(&file)==B_OK))
		{
			//fSettings.PrintToStream();
		} else {
			PRINT(("Error unflattening settings file %s\n",path.Path()));
		}	
	}
}
