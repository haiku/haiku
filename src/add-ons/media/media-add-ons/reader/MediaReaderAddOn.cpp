// MediaReaderAddOn.cpp
//
// Andrew Bachmann, 2002
//
// A MediaReaderAddOn is an add-on
// that can make MediaReader nodes
//
// MediaReader nodes read a file into a multistream


#include <MediaDefs.h>
#include <MediaAddOn.h>
#include <Errors.h>
#include <Node.h>
#include <Mime.h>
#include <StorageDefs.h>

#include "MediaReader.h"
#include "MediaReaderAddOn.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

// instantiation function
extern "C" _EXPORT BMediaAddOn * make_media_addon(image_id image) {
	return new MediaReaderAddOn(image);
}

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

MediaReaderAddOn::~MediaReaderAddOn()
{
}

MediaReaderAddOn::MediaReaderAddOn(image_id image) :
	BMediaAddOn(image)
{
	fprintf(stderr,"MediaReaderAddOn::MediaReaderAddOn\n");
	refCount = 0;
}
	
// -------------------------------------------------------- //
// BMediaAddOn impl
// -------------------------------------------------------- //

status_t MediaReaderAddOn::InitCheck(
	const char ** out_failure_text)
{
	fprintf(stderr,"MediaReaderAddOn::InitCheck\n");
	return B_OK;
}

int32 MediaReaderAddOn::CountFlavors()
{
	fprintf(stderr,"MediaReaderAddOn::CountFlavors\n");
	return 1;
}

status_t MediaReaderAddOn::GetFlavorAt(
	int32 n,
	const flavor_info ** out_info)
{
	fprintf(stderr,"MediaReaderAddOn::GetFlavorAt\n");
	if (out_info == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // we refuse to crash because you were stupid
	}	
	if (n != 0) {
		fprintf(stderr,"<- B_BAD_INDEX\n");
		return B_BAD_INDEX;
	}
	return MediaReader::GetFlavor(n,out_info);
}

BMediaNode * MediaReaderAddOn::InstantiateNodeFor(
				const flavor_info * info,
				BMessage * config,
				status_t * out_error)
{
	fprintf(stderr,"MediaReaderAddOn::InstantiateNodeFor\n");
	if (out_error == 0) {
		fprintf(stderr,"<- NULL\n");
		return 0; // we refuse to crash because you were stupid
	}
	size_t defaultChunkSize = size_t(8192); // XXX: read from add-on's attributes
	float defaultBitRate = 800000;
	MediaReader * node = new MediaReader(defaultChunkSize,
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

status_t MediaReaderAddOn::GetConfigurationFor(
				BMediaNode * your_node,
				BMessage * into_message)
{
	fprintf(stderr,"MediaReaderAddOn::GetConfigurationFor\n");
	if (into_message == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // we refuse to crash because you were stupid
	}	
	MediaReader * node = dynamic_cast<MediaReader*>(your_node);
	if (node == 0) {
		fprintf(stderr,"<- B_BAD_TYPE\n");
		return B_BAD_TYPE;
	}
	return node->GetConfigurationFor(into_message);
}

bool MediaReaderAddOn::WantsAutoStart()
{
	fprintf(stderr,"MediaReaderAddOn::WantsAutoStart\n");
	return false;
}

status_t MediaReaderAddOn::AutoStart(
				int in_count,
				BMediaNode ** out_node,
				int32 * out_internal_id,
				bool * out_has_more)
{
	fprintf(stderr,"MediaReaderAddOn::AutoStart\n");
	return B_OK;
}

// -------------------------------------------------------- //
// BMediaAddOn impl for B_FILE_INTERFACE nodes
// -------------------------------------------------------- //

status_t MediaReaderAddOn::SniffRef(
				const entry_ref & file,
				BMimeType * io_mime_type,
				float * out_quality,
				int32 * out_internal_id)
{
	fprintf(stderr,"MediaReaderAddOn::SniffRef\n");
	if ((io_mime_type == 0) || (out_quality == 0) || (out_internal_id == 0)) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // we refuse to crash because you were stupid
	}
	*out_internal_id = 0; // only one flavor
	char mime_string[B_MIME_TYPE_LENGTH+1];
	status_t status =  MediaReader::StaticSniffRef(file,mime_string,out_quality);
	io_mime_type->SetTo(mime_string);
	return status;
}

// even though I implemented SniffTypeKind below and this shouldn't get called,
// I am going to implement it anyway.  I'll use it later anyhow.
status_t MediaReaderAddOn::SniffType(
				const BMimeType & type,
				float * out_quality,
				int32 * out_internal_id)
{
	fprintf(stderr,"MediaReaderAddOn::SniffType\n");
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
	fprintf(stderr,"MediaReaderAddOn::GetFileFormatList\n");
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
			out_readable_formats[0] = MediaReader::GetFileFormat();
		}
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
	fprintf(stderr,"MediaReaderAddOn::SniffTypeKind\n");
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

status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_0(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_1(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_2(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_3(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_4(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_5(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_6(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_7(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_8(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_9(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_10(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_11(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_12(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_13(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_14(void *) {};
status_t MediaReaderAddOn::_Reserved_MediaReaderAddOn_15(void *) {};
