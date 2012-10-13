/*
 * Copyright 2012, Gerasim Troeglazov (3dEyes**), 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __VST_ADDON_H__
#define __VST_ADDON_H__

#include <MediaAddOn.h>
#include "VSTHost.h"

class VSTAddOn : public BMediaAddOn 
{
public:
	virtual		 			~VSTAddOn();
	explicit 				VSTAddOn(image_id image);
	virtual		status_t 	InitCheck(const char **text);
	virtual		int32 		CountFlavors();
	virtual		status_t 	GetFlavorAt(int32 idx, const flavor_info **info);
	virtual		BMediaNode* InstantiateNodeFor(const flavor_info *info, BMessage *config,	
							status_t *err);
	virtual		status_t 	GetConfigurationFor(BMediaNode *node, BMessage *message);
	virtual		bool 		WantsAutoStart();
	virtual		status_t 	AutoStart(int count, BMediaNode **node,	int32 *id, bool *more);
private:	
				int 		ScanPluginsFolders(const char *path, bool make_dir=false);
				BList		fPluginsList;
};

#endif //__VST_ADDON_H__
