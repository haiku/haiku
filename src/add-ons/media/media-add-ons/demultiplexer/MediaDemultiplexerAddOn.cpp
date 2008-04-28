// MediaDemultiplexerAddOn.cpp
//
// Andrew Bachmann, 2002
//
// MediaDemultiplexerAddOn is an add-on
// that can make instances of MediaDemultiplexerNode
//
// MediaDemultiplexerNode handles a file and a multistream


#include <MediaDefs.h>
#include <MediaAddOn.h>
#include <Errors.h>

#include "MediaDemultiplexerNode.h"
#include "MediaDemultiplexerAddOn.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

// instantiation function
extern "C" _EXPORT BMediaAddOn * make_media_addon(image_id image)
{
	return new MediaDemultiplexerAddOn(image);
}

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

MediaDemultiplexerAddOn::~MediaDemultiplexerAddOn()
{
}

MediaDemultiplexerAddOn::MediaDemultiplexerAddOn(image_id image) :
	BMediaAddOn(image)
{
	fprintf(stderr,"MediaDemultiplexerAddOn::MediaDemultiplexerAddOn\n");
	refCount = 0;
}
	
// -------------------------------------------------------- //
// BMediaAddOn impl
// -------------------------------------------------------- //

status_t MediaDemultiplexerAddOn::InitCheck(
	const char ** out_failure_text)
{
	fprintf(stderr,"MediaDemultiplexerAddOn::InitCheck\n");
	return B_OK;
}

int32 MediaDemultiplexerAddOn::CountFlavors()
{
	fprintf(stderr,"MediaDemultiplexerAddOn::CountFlavors\n");
	return 1;
}

status_t MediaDemultiplexerAddOn::GetFlavorAt(
	int32 n,
	const flavor_info ** out_info)
{
	fprintf(stderr,"MediaDemultiplexerAddOn::GetFlavorAt\n");
	if (n != 0) {
		fprintf(stderr,"<- B_BAD_INDEX\n");
		return B_BAD_INDEX;
	}
	flavor_info * infos = new flavor_info[1];
	MediaDemultiplexerNode::GetFlavor(&infos[0],n);
	(*out_info) = infos;
	return B_OK;
}

BMediaNode * MediaDemultiplexerAddOn::InstantiateNodeFor(
				const flavor_info * info,
				BMessage * config,
				status_t * out_error)
{
	fprintf(stderr,"MediaDemultiplexerAddOn::InstantiateNodeFor\n");
	MediaDemultiplexerNode * node
		= new MediaDemultiplexerNode(info,config,this);
	if (node == 0) {
		*out_error = B_NO_MEMORY;
		fprintf(stderr,"<- B_NO_MEMORY\n");
	} else { 
		*out_error = node->InitCheck();
	}
	return node;	
}

status_t MediaDemultiplexerAddOn::GetConfigurationFor(
				BMediaNode * your_node,
				BMessage * into_message)
{
	fprintf(stderr,"MediaDemultiplexerAddOn::GetConfigurationFor\n");
	MediaDemultiplexerNode * node
		= dynamic_cast<MediaDemultiplexerNode*>(your_node);
	if (node == 0) {
		fprintf(stderr,"<- B_BAD_TYPE\n");
		return B_BAD_TYPE;
	}
	return node->GetConfigurationFor(into_message);
}

bool MediaDemultiplexerAddOn::WantsAutoStart()
{
	fprintf(stderr,"MediaDemultiplexerAddOn::WantsAutoStart\n");
	return false;
}

status_t MediaDemultiplexerAddOn::AutoStart(
				int in_count,
				BMediaNode ** out_node,
				int32 * out_internal_id,
				bool * out_has_more)
{
	fprintf(stderr,"MediaDemultiplexerAddOn::AutoStart\n");
	return B_OK;
}

// -------------------------------------------------------- //
// main
// -------------------------------------------------------- //

int main(int argc, char *argv[])
{
	fprintf(stderr,"main called for MediaDemultiplexerAddOn\n");
}

// -------------------------------------------------------- //
// stuffing
// -------------------------------------------------------- //

status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_0(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_1(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_2(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_3(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_4(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_5(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_6(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_7(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_8(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_9(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_10(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_11(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_12(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_13(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_14(void *) {};
status_t MediaDemultiplexerAddOn::_Reserved_MediaDemultiplexerAddOn_15(void *) {};
