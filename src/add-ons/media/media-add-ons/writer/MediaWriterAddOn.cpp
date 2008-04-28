// MediaWriterAddOn.cpp
//
// Andrew Bachmann, 2002
//
// A MediaWriterAddOn is an add-on
// that can make MediaWriter nodes
//
// MediaWriter nodes write a multistream into a file


#include <MediaDefs.h>
#include <MediaAddOn.h>
#include <Errors.h>
#include <Node.h>
#include <Mime.h>
#include <StorageDefs.h>

#include "MediaWriter.h"
#include "MediaWriterAddOn.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

// instantiation function
extern "C" _EXPORT BMediaAddOn * make_media_addon(image_id image) {
	return new MediaWriterAddOn(image);
}

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

MediaWriterAddOn::~MediaWriterAddOn()
{
}

MediaWriterAddOn::MediaWriterAddOn(image_id image) :
	AbstractFileInterfaceAddOn(image)
{
	fprintf(stderr,"MediaWriterAddOn::MediaWriterAddOn\n");
}

// -------------------------------------------------------- //
// BMediaAddOn impl
// -------------------------------------------------------- //

status_t MediaWriterAddOn::GetFlavorAt(
	int32 n,
	const flavor_info ** out_info)
{
	fprintf(stderr,"MediaWriterAddOn::GetFlavorAt\n");
	if (n != 0) {
		fprintf(stderr,"<- B_BAD_INDEX\n");
		return B_BAD_INDEX;
	}
	flavor_info * infos = new flavor_info[1];
	MediaWriter::GetFlavor(&infos[0],n);
	(*out_info) = infos;
	return B_OK;
}

BMediaNode * MediaWriterAddOn::InstantiateNodeFor(
				const flavor_info * info,
				BMessage * config,
				status_t * out_error)
{
	fprintf(stderr,"MediaWriterAddOn::InstantiateNodeFor\n");
	size_t defaultChunkSize = size_t(8192); // XXX: read from add-on's attributes
	float defaultBitRate = 800000;
	MediaWriter * node
		= new MediaWriter(defaultChunkSize,
						  defaultBitRate,
						  info,config,this);
	if (node == 0) {
		*out_error = B_NO_MEMORY;
		fprintf(stderr,"<- B_NO_MEMORY\n");
	} else { 
		*out_error = node->InitCheck();
	}
	return node;	
}

status_t MediaWriterAddOn::GetConfigurationFor(
				BMediaNode * your_node,
				BMessage * into_message)
{
	fprintf(stderr,"MediaWriterAddOn::GetConfigurationFor\n");
	MediaWriter * node
		= dynamic_cast<MediaWriter*>(your_node);
	if (node == 0) {
		fprintf(stderr,"<- B_BAD_TYPE\n");
		return B_BAD_TYPE;
	}
	return node->GetConfigurationFor(into_message);
}

// -------------------------------------------------------- //
// BMediaAddOn impl for B_FILE_INTERFACE nodes
// -------------------------------------------------------- //

status_t MediaWriterAddOn::GetFileFormatList(
				int32 flavor_id,
				media_file_format * out_writable_formats,
				int32 in_write_items,
				int32 * out_write_items,
				media_file_format * out_readable_formats,
				int32 in_read_items,
				int32 * out_read_items,
				void * _reserved)
{
	fprintf(stderr,"MediaWriterAddOn::GetFileFormatList\n");
	if (flavor_id != 0) {
		// this is a sanity check for now
		fprintf(stderr,"<- B_BAD_INDEX\n");
		return B_BAD_INDEX;
	}
		// don't go off the end
		if (in_write_items > 0) {
			MediaWriter::GetFileFormat(&out_writable_formats[0]);
		}
	return B_OK;
}

status_t MediaWriterAddOn::SniffTypeKind(
				const BMimeType & type,
				uint64 in_kinds,
				float * out_quality,
				int32 * out_internal_id,
				void * _reserved)
{
	fprintf(stderr,"MediaWriterAddOn::SniffTypeKind\n");
	return AbstractFileInterfaceAddOn::SniffTypeKind(type,in_kinds,
													 B_BUFFER_CONSUMER,
													 out_quality,out_internal_id,
													 _reserved);
}

// -------------------------------------------------------- //
// main
// -------------------------------------------------------- //

int main(int argc, char *argv[])
{
	fprintf(stderr,"main called for MediaWriterAddOn\n");
}

// -------------------------------------------------------- //
// stuffing
// -------------------------------------------------------- //

status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_0(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_1(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_2(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_3(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_4(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_5(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_6(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_7(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_8(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_9(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_10(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_11(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_12(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_13(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_14(void *) {};
status_t MediaWriterAddOn::_Reserved_MediaWriterAddOn_15(void *) {};
