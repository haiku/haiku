// MediaFileProducerAddOn.cpp
//
// Andrew Bachmann, 2002
//
// A MediaFileProducerAddOn is an add-on
// that can make MediaFileProducer nodes
//
// MediaFileProducer nodes read a file into a multistream


#include <MediaDefs.h>
#include <MediaAddOn.h>
#include <Errors.h>
#include <Node.h>
#include <Mime.h>
#include <StorageDefs.h>

#include "MediaFileProducer.h"
#include "MediaFileProducerAddOn.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

// instantiation function
extern "C" _EXPORT BMediaAddOn * make_media_addon(image_id image) {
	return new MediaFileProducerAddOn(image);
}

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

MediaFileProducerAddOn::~MediaFileProducerAddOn()
{
}

MediaFileProducerAddOn::MediaFileProducerAddOn(image_id image) :
	BMediaAddOn(image)
{
	fprintf(stderr,"MediaFileProducerAddOn::MediaFileProducerAddOn\n");
	refCount = 0;
}
	
// -------------------------------------------------------- //
// BMediaAddOn impl
// -------------------------------------------------------- //

status_t MediaFileProducerAddOn::InitCheck(
	const char ** out_failure_text)
{
	fprintf(stderr,"MediaFileProducerAddOn::InitCheck\n");
	return B_OK;
}

int32 MediaFileProducerAddOn::CountFlavors()
{
	fprintf(stderr,"MediaFileProducerAddOn::CountFlavors\n");
	return 1;
}

status_t MediaFileProducerAddOn::GetFlavorAt(
	int32 n,
	const flavor_info ** out_info)
{
	fprintf(stderr,"MediaFileProducerAddOn::GetFlavorAt\n");
	if (out_info == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // we refuse to crash because you were stupid
	}	
	if (n != 0) {
		fprintf(stderr,"<- B_BAD_INDEX\n");
		return B_BAD_INDEX;
	}
	return MediaFileProducer::GetFlavor(n,out_info);
}

BMediaNode * MediaFileProducerAddOn::InstantiateNodeFor(
				const flavor_info * info,
				BMessage * config,
				status_t * out_error)
{
	fprintf(stderr,"MediaFileProducerAddOn::InstantiateNodeFor\n");
	if (out_error == 0) {
		fprintf(stderr,"<- NULL\n");
		return 0; // we refuse to crash because you were stupid
	}
	size_t defaultChunkSize = size_t(8192); // XXX: read from add-on's attributes
	float defaultBitRate = 800000;
	MediaFileProducer * node = new MediaFileProducer(defaultChunkSize,
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

status_t MediaFileProducerAddOn::GetConfigurationFor(
				BMediaNode * your_node,
				BMessage * into_message)
{
	fprintf(stderr,"MediaFileProducerAddOn::GetConfigurationFor\n");
	if (into_message == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // we refuse to crash because you were stupid
	}	
	MediaFileProducer * node = dynamic_cast<MediaFileProducer*>(your_node);
	if (node == 0) {
		fprintf(stderr,"<- B_BAD_TYPE\n");
		return B_BAD_TYPE;
	}
	return node->GetConfigurationFor(into_message);
}

bool MediaFileProducerAddOn::WantsAutoStart()
{
	fprintf(stderr,"MediaFileProducerAddOn::WantsAutoStart\n");
	return false;
}

status_t MediaFileProducerAddOn::AutoStart(
				int in_count,
				BMediaNode ** out_node,
				int32 * out_internal_id,
				bool * out_has_more)
{
	fprintf(stderr,"MediaFileProducerAddOn::AutoStart\n");
	return B_OK;
}

// -------------------------------------------------------- //
// BMediaAddOn impl for B_FILE_INTERFACE nodes
// -------------------------------------------------------- //

status_t MediaFileProducerAddOn::SniffRef(
				const entry_ref & file,
				BMimeType * io_mime_type,
				float * out_quality,
				int32 * out_internal_id)
{
	fprintf(stderr,"MediaFileProducerAddOn::SniffRef\n");
	if ((io_mime_type == 0) || (out_quality == 0) || (out_internal_id == 0)) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // we refuse to crash because you were stupid
	}
	*out_internal_id = 0; // only one flavor
	char mime_string[B_MIME_TYPE_LENGTH+1];
	status_t status =  MediaFileProducer::StaticSniffRef(file,mime_string,out_quality);
	io_mime_type->SetTo(mime_string);
	return status;
}

// even though I implemented SniffTypeKind below and this shouldn't get called,
// I am going to implement it anyway.  I'll use it later anyhow.
status_t MediaFileProducerAddOn::SniffType(
				const BMimeType & type,
				float * out_quality,
				int32 * out_internal_id)
{
	fprintf(stderr,"MediaFileProducerAddOn::SniffType\n");
	if ((out_quality == 0) || (out_internal_id == 0)) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // we refuse to crash because you were stupid
	}	
	*out_quality = 1.0;
	*out_internal_id = 0;
	return B_OK;	
}

// This function treats null pointers slightly differently than the others.
// This is because a program could reasonably call this function with just
// about any junk, get the out_read_items and then use that to create an
// array of sufficient size to hold the result, and then recall.  Also, a
// stupid program could not supply an out_read_items, but actually supply
// an out_readable_formats and then try to do something useful with it. As
// an extreme gesture of nicety we will fill the out_readable_formats with
// a valid entry, although they could easily read into garbage after that...
status_t MediaFileProducerAddOn::GetFileFormatList(
				int32 flavor_id,
				media_file_format * out_writable_formats,
				int32 in_write_items,
				int32 * out_write_items,
				media_file_format * out_readable_formats,
				int32 in_read_items,
				int32 * out_read_items,
				void * _reserved)
{
	fprintf(stderr,"MediaFileProducerAddOn::GetFileFormatList\n");
	if (flavor_id != 0) {
		// this is a sanity check for now
		fprintf(stderr,"<- B_BAD_INDEX\n");
		return B_BAD_INDEX;
	}
	// see null check comment above
	if (out_write_items != 0) {
		*out_write_items = 0;
	}	
	// see null check comment above
	if (out_read_items != 0) {
		*out_read_items = 1;
	}
	// see null check comment above
	if (out_readable_formats != 0) {
		// don't go off the end
		if (in_read_items > 0) {
			out_readable_formats[0] = MediaFileProducer::GetFileFormat();
		}
	}
	return B_OK;
}

status_t MediaFileProducerAddOn::SniffTypeKind(
				const BMimeType & type,
				uint64 in_kinds,
				float * out_quality,
				int32 * out_internal_id,
				void * _reserved)
{
	fprintf(stderr,"MediaFileProducerAddOn::SniffTypeKind\n");
	if ((out_quality == 0) || (out_internal_id == 0)) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // we refuse to crash because you were stupid
	}
	if (in_kinds & (B_BUFFER_PRODUCER | B_FILE_INTERFACE | B_CONTROLLABLE)) {
		return SniffType(type,out_quality,out_internal_id);
	} else {
		// They asked for some kind we don't supply.  We set the output
		// just in case they try to do something with it anyway (!)
		*out_quality = 0;
		*out_internal_id = -1; 
		fprintf(stderr,"<- B_BAD_TYPE\n");
		return B_BAD_TYPE;
	}
}

// -------------------------------------------------------- //
// stuffing
// -------------------------------------------------------- //

int main(int argc, char *argv[])
{

}

// -------------------------------------------------------- //
// stuffing
// -------------------------------------------------------- //

status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_0(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_1(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_2(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_3(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_4(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_5(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_6(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_7(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_8(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_9(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_10(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_11(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_12(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_13(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_14(void *) {};
status_t MediaFileProducerAddOn::_Reserved_MediaFileProducerAddOn_15(void *) {};
