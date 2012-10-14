/*
 * Copyright 2012, Gerasim Troeglazov (3dEyes**), 3dEyes@gmail.com.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */ 

#include <Entry.h>
#include <Directory.h>
#include <FindDirectory.h>

#include "VSTNode.h"
#include "VSTAddOn.h"

extern "C" _EXPORT BMediaAddOn* make_media_addon(image_id image)
{
	return new VSTAddOn(image);
}

VSTAddOn::VSTAddOn(image_id image) : BMediaAddOn(image)
{
	const char vst_subdir[]={"media/vstplugins"};

	BPath addons_dir;
	fPluginsList.MakeEmpty();

	find_directory(B_COMMON_ADDONS_DIRECTORY, &addons_dir);
	addons_dir.Append(vst_subdir);
	ScanPluginsFolders(addons_dir.Path(), true);

	find_directory(B_USER_ADDONS_DIRECTORY, &addons_dir);
	addons_dir.Append(vst_subdir);
	ScanPluginsFolders(addons_dir.Path() ,true);

	find_directory(B_BEOS_ADDONS_DIRECTORY, &addons_dir);
	addons_dir.Append(vst_subdir);	
	ScanPluginsFolders(addons_dir.Path());
}

VSTAddOn::~VSTAddOn()
{	
	
}
	
status_t 
VSTAddOn::InitCheck(const char** text) 
{
	return B_OK;
}
	
int32 
VSTAddOn::CountFlavors()
{
	return fPluginsList.CountItems();
}

status_t
VSTAddOn::GetFlavorAt(int32 idx, const flavor_info** info) 
{
	if (idx < 0 || idx >= fPluginsList.CountItems())
		return B_ERROR;
		
	VSTPlugin *plugin = (VSTPlugin*)fPluginsList.ItemAt(idx);	
	
	flavor_info *f_info = new flavor_info;
	f_info->internal_id = idx;
	f_info->kinds = B_BUFFER_CONSUMER | B_BUFFER_PRODUCER | B_CONTROLLABLE;
	f_info->possible_count = 0;
	f_info->flavor_flags = 0;
	f_info->name = (char *)plugin->ModuleName();
	f_info->info = (char *)plugin->Product();
	
	media_format *format = new media_format;
	format->type = B_MEDIA_RAW_AUDIO;
	format->u.raw_audio = media_raw_audio_format::wildcard;
	format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	
	f_info->in_format_count = 1;
	f_info->in_formats = format;

	format = new media_format;
	format->type = B_MEDIA_RAW_AUDIO;
	format->u.raw_audio = media_raw_audio_format::wildcard;
	format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;

	f_info->out_format_count = 1;
	f_info->out_formats = format;		
	
	*info = f_info;
	
	return B_OK;
}

BMediaNode* 
VSTAddOn::InstantiateNodeFor(const flavor_info* info, BMessage* config,
								  status_t* err)
{
	VSTPlugin* plugin = (VSTPlugin*)fPluginsList.ItemAt(info->internal_id);
	VSTNode* node = new VSTNode(this, plugin->ModuleName(), plugin->Path());
	return node;
}

int
VSTAddOn::ScanPluginsFolders(const char* path, bool make_dir)
{
	BEntry ent;

	BDirectory dir(path);
	if (dir.InitCheck() != B_OK) {
		create_directory(path, 0755);
		return 0;
	}

	while(dir.GetNextEntry(&ent) == B_OK) {
		BPath p(&ent);
		if (ent.IsDirectory()) {
			ScanPluginsFolders(p.Path());
		} else {
			VSTPlugin* plugin = new VSTPlugin();
			int ret = plugin->LoadModule(p.Path());
			if (ret == B_OK) {
				plugin->UnLoadModule();
				fPluginsList.AddItem(plugin);				
			} else {
				delete plugin;
			}
		}
	}
	return 0;
}

status_t 
VSTAddOn::GetConfigurationFor(BMediaNode* node, BMessage* message)
{
	return B_OK;
}

bool
VSTAddOn::WantsAutoStart()
{ 
	return false; 
}

status_t 	
VSTAddOn::AutoStart(int count, BMediaNode** node, int32* id, bool* more)
{ 
	return B_OK; 
}
