// MediaReaderAddOn.cpp
//
// Andrew Bachmann, 2002
//
// A MediaReaderAddOn is an add-on
// that can make MediaReader nodes
//
// MediaReader nodes read a file into a multistream
#include "MediaReader.h"
#include "MediaReaderAddOn.h"
#include "debug.h"

#include <Errors.h>
#include <MediaAddOn.h>
#include <MediaDefs.h>
#include <MediaRoster.h>
#include <Mime.h>
#include <Node.h>
#include <StorageDefs.h>


#include <limits.h>
#include <stdio.h>
#include <string.h>


// instantiation function
extern "C" _EXPORT BMediaAddOn * make_media_addon(image_id image) {
	return new MediaReaderAddOn(image);
}


MediaReaderAddOn::~MediaReaderAddOn()
{
}


MediaReaderAddOn::MediaReaderAddOn(image_id image) :
	AbstractFileInterfaceAddOn(image)
{
	CALLED();
}


// -------------------------------------------------------- //
// BMediaAddOn impl
// -------------------------------------------------------- //
status_t MediaReaderAddOn::GetFlavorAt(
	int32 n,
	const flavor_info ** out_info)
{
	CALLED();

	if (n != 0) {
		PRINT("\t<- B_BAD_INDEX\n");
		return B_BAD_INDEX;
	}

	flavor_info * infos = new flavor_info[1];
	MediaReader::GetFlavor(&infos[0],n);
	(*out_info) = infos;
	return B_OK;
}


BMediaNode * MediaReaderAddOn::InstantiateNodeFor(
				const flavor_info * info,
				BMessage * config,
				status_t * out_error)
{
	CALLED();

	// XXX: read from add-on's attributes
	size_t defaultChunkSize = size_t(8192); // 8192 bytes = 8 Kilobytes
	// = 2048 kilobits/millisec = 256000 Kilobytes/sec
	float defaultBitRate = 2048; 
	MediaReader * node
		= new MediaReader(defaultChunkSize,
						  defaultBitRate,
						  info,config,this);
	if (node == 0) {
		*out_error = B_NO_MEMORY;
		PRINT("\t<- B_NO_MEMORY\n");
	} else { 
		*out_error = node->InitCheck();
	}
	return node;	
}


status_t MediaReaderAddOn::GetConfigurationFor(
				BMediaNode * your_node,
				BMessage * into_message)
{
	CALLED();

	MediaReader * node
		= dynamic_cast<MediaReader*>(your_node);
	if (node == 0) {
		fprintf(stderr,"<- B_BAD_TYPE\n");
		return B_BAD_TYPE;
	}
	return node->GetConfigurationFor(into_message);
}


// -------------------------------------------------------- //
// BMediaAddOn impl for B_FILE_INTERFACE nodes
// -------------------------------------------------------- //

status_t MediaReaderAddOn::GetFileFormatList(
				int32 flavor_id,
				media_file_format * out_writable_formats,
				int32 in_write_items,
				int32 * out_write_items,
				media_file_format * out_readable_formats,
				int32 in_read_items,
				int32 * out_read_items,
				void * _reserved)
{
	CALLED();

	if (flavor_id != 0) {
		// this is a sanity check for now
		PRINT("\t<- B_BAD_INDEX\n");
		return B_BAD_INDEX;
	}
		// don't go off the end
		if (in_read_items > 0) {
			MediaReader::GetFileFormat(&out_readable_formats[0]);
		}
	return B_OK;
}


status_t MediaReaderAddOn::SniffTypeKind(
				const BMimeType & type,
				uint64 in_kinds,
				float * out_quality,
				int32 * out_internal_id,
				void * _reserved)
{
	CALLED();
	return AbstractFileInterfaceAddOn::SniffTypeKind(type,in_kinds,
													 B_BUFFER_PRODUCER,
													 out_quality,out_internal_id,
													 _reserved);
}

// -------------------------------------------------------- //
// main
// -------------------------------------------------------- //
int main(int argc, char *argv[])
{
	fprintf(stderr,"main called for MediaReaderAddOn\n");
	return 0;
}

// -------------------------------------------------------- //
// stuffing
// -------------------------------------------------------- //

status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_0(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_1(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_2(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_3(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_4(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_5(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_6(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_7(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_8(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_9(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_10(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_11(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_12(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_13(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_14(void *) {return B_ERROR;};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_15(void *) {return B_ERROR;};
