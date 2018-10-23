/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: FileInterface.cpp
 *  DESCR:
 ***********************************************************************/
/*
 * Copyright 2008 Maurice Kalinowski, haiku@kaldience.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include "MediaDebug.h"
#include "DataExchange.h"
#include <string.h>
#include <FileInterface.h>
#include <MimeType.h>


/*************************************************************
 * protected BFileInterface
 *************************************************************/

BFileInterface::~BFileInterface()
{
}

/*************************************************************
 * public BFileInterface
 *************************************************************/

/* nothing */

/*************************************************************
 * protected BFileInterface
 *************************************************************/

BFileInterface::BFileInterface()
	: BMediaNode("called by FileInterface")
{
	CALLED();

	AddNodeKind(B_FILE_INTERFACE);
}


status_t
BFileInterface::HandleMessage(int32 message,
							  const void *data,
							  size_t size)
{
	CALLED();

	status_t rv;

	switch(message) {
		case FILEINTERFACE_SET_REF:
		{
			const fileinterface_set_ref_request *request =
					(const fileinterface_set_ref_request*) data;
			fileinterface_set_ref_reply reply;
			entry_ref ref(request->device, request->directory,
								request->name);
			reply.duration = request->duration;

			rv = SetRef(ref, request->create, &reply.duration);

			request->SendReply(rv, &reply, sizeof(reply));
			return B_OK;
		}
		case FILEINTERFACE_GET_REF:
		{
			const fileinterface_get_ref_request *request =
					(const fileinterface_get_ref_request*) data;
			fileinterface_get_ref_reply reply;
			entry_ref resultRef;
			rv = GetRef(&resultRef, reply.mimetype);
			if (rv == B_OK) {
				reply.device = resultRef.device;
				reply.directory = resultRef.directory;
				strcpy(reply.name, resultRef.name);
			}
			request->SendReply(rv, &reply, sizeof(reply));
			return B_OK;
		}
		case FILEINTERFACE_SNIFF_REF:
		{
			const fileinterface_sniff_ref_request *request =
					(const fileinterface_sniff_ref_request*) data;
			fileinterface_sniff_ref_reply reply;

			entry_ref ref(request->device, request->directory,
						  request->name);

			rv = SniffRef(ref, reply.mimetype, &reply.capability);
			request->SendReply(rv, &reply, sizeof(reply));

			return B_OK;
		}
		case FILEINTERFACE_GET_FORMATS:
		{
			const fileinterface_get_formats_request *request =
					(const fileinterface_get_formats_request*) data;
			fileinterface_get_formats_reply reply;

			media_file_format* formats;
			area_id area = clone_area("client formats area",
				(void**)&formats, B_ANY_ADDRESS, B_WRITE_AREA,
				request->data_area);

			if (area < 0) {
				ERROR("BBufferConsumer::FILEINTERFACE_GET_FORMATS:"
					" can't clone area\n");
				break;
			}

			int32 cookie = 0;
			while (GetNextFileFormat(&cookie, formats) == B_OK) {
				if (cookie >= request->num_formats)
					break;
				formats += sizeof(media_format);
			}
			reply.filled_slots = cookie;
			request->SendReply(B_OK, &reply, sizeof(reply));

			delete_area(area);
			return B_OK;
		}
		default:
			return B_ERROR;
	}
	return B_ERROR;
}

/*************************************************************
 * private BFileInterface
 *************************************************************/

/*
private unimplemented
BFileInterface::BFileInterface(const BFileInterface &clone)
FileInterface & BFileInterface::operator=(const BFileInterface &clone)
*/

status_t BFileInterface::_Reserved_FileInterface_0(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_1(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_2(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_3(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_4(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_5(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_6(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_7(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_8(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_9(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_10(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_11(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_12(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_13(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_14(void *) { return B_ERROR; }
status_t BFileInterface::_Reserved_FileInterface_15(void *) { return B_ERROR; }

