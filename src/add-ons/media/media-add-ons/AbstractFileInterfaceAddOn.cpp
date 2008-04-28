// AbstractFileInterfaceAddOn.cpp
//
// Andrew Bachmann, 2002
//
// AbstractFileInterfaceAddOn is an add-on
// that can make instances of AbstractFileInterfaceNode
//
// AbstractFileInterfaceNode handles a file and a multistream


#include "AbstractFileInterfaceNode.h"
#include "AbstractFileInterfaceAddOn.h"
#include "debug.h"

#include <Errors.h>
#include <MediaDefs.h>
#include <MediaAddOn.h>
#include <Mime.h>
#include <Node.h>
#include <StorageDefs.h>


#include <limits.h>
#include <stdio.h>
#include <string.h>


AbstractFileInterfaceAddOn::~AbstractFileInterfaceAddOn()
{
}


AbstractFileInterfaceAddOn::AbstractFileInterfaceAddOn(image_id image) :
	BMediaAddOn(image)
{
	CALLED();
	refCount = 0;
}


// -------------------------------------------------------- //
// BMediaAddOn impl
// -------------------------------------------------------- //
status_t AbstractFileInterfaceAddOn::InitCheck(
	const char ** out_failure_text)
{
	CALLED();
	return B_OK;
}

int32 AbstractFileInterfaceAddOn::CountFlavors()
{
	CALLED();
	return 1;
}

status_t AbstractFileInterfaceAddOn::GetFlavorAt(
	int32 n,
	const flavor_info ** out_info)
{
	CALLED();

	if (n != 0) {
		PRINT("\t<- B_BAD_INDEX\n");
		return B_BAD_INDEX;
	}

	flavor_info * infos = new flavor_info[1];
	AbstractFileInterfaceNode::GetFlavor(&infos[0],n);
	(*out_info) = infos;
	return B_OK;
}


status_t AbstractFileInterfaceAddOn::GetConfigurationFor(
				BMediaNode * your_node,
				BMessage * into_message)
{
	CALLED();
	AbstractFileInterfaceNode * node
		= dynamic_cast<AbstractFileInterfaceNode*>(your_node);
	if (node == 0) {
		PRINT("\t<- B_BAD_TYPE\n");
		return B_BAD_TYPE;
	}
	return node->GetConfigurationFor(into_message);
}


bool AbstractFileInterfaceAddOn::WantsAutoStart()
{
	CALLED();
	return false;
}


status_t AbstractFileInterfaceAddOn::AutoStart(
				int in_count,
				BMediaNode ** out_node,
				int32 * out_internal_id,
				bool * out_has_more)
{
	CALLED();
	return B_OK;
}


// -------------------------------------------------------- //
// BMediaAddOn impl for B_FILE_INTERFACE nodes
// -------------------------------------------------------- //
status_t AbstractFileInterfaceAddOn::SniffRef(
				const entry_ref & file,
				BMimeType * io_mime_type,
				float * out_quality,
				int32 * out_internal_id)
{
	CALLED();

	*out_internal_id = 0; // only one flavor
	char mime_string[B_MIME_TYPE_LENGTH+1];
	status_t status =  AbstractFileInterfaceNode::StaticSniffRef(file,mime_string,out_quality);
	io_mime_type->SetTo(mime_string);
	return status;
}


// even though I implemented SniffTypeKind below and this shouldn't get called,
// I am going to implement it anyway.  I'll use it later anyhow.
status_t AbstractFileInterfaceAddOn::SniffType(
				const BMimeType & type,
				float * out_quality,
				int32 * out_internal_id)
{
	CALLED();

	*out_quality = 1.0;
	*out_internal_id = 0;
	return B_OK;	
}


status_t AbstractFileInterfaceAddOn::GetFileFormatList(
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

	*out_write_items = 0;
	*out_read_items = 0;

	return B_OK;
}


status_t AbstractFileInterfaceAddOn::SniffTypeKind(
				const BMimeType & type,
				uint64 in_kinds,
				uint64 io_kind,
				float * out_quality,
				int32 * out_internal_id,
				void * _reserved)
{
	CALLED();

	if (in_kinds & (io_kind | B_FILE_INTERFACE | B_CONTROLLABLE)) {
		return SniffType(type,out_quality,out_internal_id);
	} else {
		// They asked for some kind we don't supply.  We set the output
		// just in case they try to do something with it anyway (!)
		*out_quality = 0;
		*out_internal_id = -1; 
		PRINT("\t<- B_BAD_TYPE\n");
		return B_BAD_TYPE;
	}
}


// -------------------------------------------------------- //
// main
// -------------------------------------------------------- //

// int main(int argc, char *argv[])
//{
//}

// -------------------------------------------------------- //
// stuffing
// -------------------------------------------------------- //

status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_0(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_1(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_2(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_3(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_4(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_5(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_6(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_7(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_8(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_9(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_10(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_11(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_12(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_13(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_14(void *) {return B_ERROR;};
status_t AbstractFileInterfaceAddOn::_Reserved_AbstractFileInterfaceAddOn_15(void *) {return B_ERROR;};
