/*
* Copyright (C) 2009-2010 David McPaul
*
* All rights reserved. Distributed under the terms of the MIT License.
* VideoMixerAddOn.cpp
*
* The VideoMixerAddOn class
* makes instances of VideoMixerNode
*/


#include <Debug.h>
#include <MediaDefs.h>
#include <MediaAddOn.h>
#include <Errors.h>

#include "VideoMixerNode.h"
#include "VideoMixerAddOn.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define DEBUG 1

// instantiation function
extern "C" _EXPORT BMediaAddOn *make_media_addon(image_id image)
{
	return new VideoMixerAddOn(image);
}

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

VideoMixerAddOn::~VideoMixerAddOn()
{
}

VideoMixerAddOn::VideoMixerAddOn(image_id image) :
	BMediaAddOn(image)
{
	PRINT("VideoMixerAddOn::VideoMixerAddOn\n");
	refCount = 0;
}
	
// -------------------------------------------------------- //
// BMediaAddOn impl
// -------------------------------------------------------- //

status_t VideoMixerAddOn::InitCheck(
	const char **out_failure_text)
{
	PRINT("VideoMixerAddOn::InitCheck\n");
	return B_OK;
}

int32 VideoMixerAddOn::CountFlavors()
{
	PRINT("VideoMixerAddOn::CountFlavors\n");
	return 1;
}

status_t VideoMixerAddOn::GetFlavorAt(
	int32 n,
	const flavor_info **out_info)
{
	PRINT("VideoMixerAddOn::GetFlavorAt\n");
	if (n != 0) {
		fprintf(stderr,"<- B_BAD_INDEX\n");
		return B_BAD_INDEX;
	}
	flavor_info *infos = new flavor_info[1];
	VideoMixerNode::GetFlavor(&infos[0], n);
	(*out_info) = infos;
	return B_OK;
}

BMediaNode * VideoMixerAddOn::InstantiateNodeFor(
				const flavor_info * info,
				BMessage * config,
				status_t * out_error)
{
	PRINT("VideoMixerAddOn::InstantiateNodeFor\n");
	VideoMixerNode *node = new VideoMixerNode(info, config, this);
	if (node == NULL) {
		*out_error = B_NO_MEMORY;
		fprintf(stderr,"<- B_NO_MEMORY\n");
	} else { 
		*out_error = node->InitCheck();
	}
	return node;	
}

status_t VideoMixerAddOn::GetConfigurationFor(
				BMediaNode * your_node,
				BMessage * into_message)
{
	PRINT("VideoMixerAddOn::GetConfigurationFor\n");
	VideoMixerNode *node = dynamic_cast<VideoMixerNode *>(your_node);
	if (node == NULL) {
		fprintf(stderr,"<- B_BAD_TYPE\n");
		return B_BAD_TYPE;
	}
	return node->GetConfigurationFor(into_message);
}

bool VideoMixerAddOn::WantsAutoStart()
{
	PRINT("VideoMixerAddOn::WantsAutoStart\n");
	return false;
}

status_t VideoMixerAddOn::AutoStart(
				int in_count,
				BMediaNode **out_node,
				int32 *out_internal_id,
				bool *out_has_more)
{
	PRINT("VideoMixerAddOn::AutoStart\n");
	return B_OK;
}

// -------------------------------------------------------- //
// main
// -------------------------------------------------------- //

int main(int argc, char *argv[])
{
	fprintf(stderr,"VideoMixerAddOn cannot be run\n");
}
