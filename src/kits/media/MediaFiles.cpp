/***********************************************************************
 * AUTHOR: Marcus Overhagen, Jérôme Duval
 *   FILE: MediaFiles.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaFiles.h>
#include "DataExchange.h"
#include "debug.h"

/*************************************************************
 * public BMediaFiles
 *************************************************************/

const char BMediaFiles::B_SOUNDS[] = "Sounds";	/* for "types" */

BMediaFiles::BMediaFiles()
	: m_type_index(-1),
	m_cur_type(""),
	m_item_index(-1)
{
	
}


/* virtual */ 
BMediaFiles::~BMediaFiles()
{
	
}


/* virtual */ status_t
BMediaFiles::RewindTypes()
{
	CALLED();
	status_t rv;
	server_rewindtypes_request request;
	server_rewindtypes_reply reply;
	
	for(int32 i = 0; i < m_types.CountItems(); i++)
		delete m_types.ItemAt(i);
		
	m_types.MakeEmpty();

	TRACE("BMediaFiles::RewindTypes: sending SERVER_REWINDTYPES\n");
	
	rv = QueryServer(SERVER_REWINDTYPES, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK) {
		ERROR("BMediaFiles::RewindTypes: failed to rewindtypes (error %#lx)\n", rv);
		return rv;
	}

	char *types;
	area_id clone;

	clone = clone_area("rewind types clone", reinterpret_cast<void **>(&types), B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, reply.area);
	if (clone < B_OK) {
		ERROR("BMediaFiles::RewindTypes failed to clone area, %#lx\n", clone);
		delete_area(reply.area);
		return B_ERROR;
	}

	for (int32 i = 0; i < reply.count; i++) {
		m_types.AddItem(new BString(types + i * B_MEDIA_NAME_LENGTH));
	}

	delete_area(clone);
	delete_area(reply.area);
	
	m_type_index = 0;

	return B_OK;
}


/* virtual */ status_t
BMediaFiles::GetNextType(BString *out_type)
{
	CALLED();
	if(m_type_index < 0 || m_type_index >= m_types.CountItems()) {
		m_type_index = -1;
		return B_BAD_INDEX;
	}
	
	*out_type = *(BString*)m_types.ItemAt(m_type_index);
	m_type_index++;

	return B_OK;
}


/* virtual */ status_t
BMediaFiles::RewindRefs(const char *type)
{
	CALLED();
	status_t rv;
	server_rewindrefs_request request;
	server_rewindrefs_reply reply;
	
	for(int32 i = 0; i < m_items.CountItems(); i++)
		delete m_items.ItemAt(i);
		
	m_items.MakeEmpty();

	TRACE("BMediaFiles::RewindRefs: sending SERVER_REWINDREFS for type %s\n", type);
	strncpy(request.type, type, B_MEDIA_NAME_LENGTH);
	
	rv = QueryServer(SERVER_REWINDREFS, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK) {
		ERROR("BMediaFiles::RewindRefs: failed to rewindrefs (error %#lx)\n", rv);
		return rv;
	}

	if(reply.count>0) {
		char *items;
		area_id clone;
	
		clone = clone_area("rewind refs clone", reinterpret_cast<void **>(&items), B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, reply.area);
		if (clone < B_OK) {
			ERROR("BMediaFiles::RewindRefs failed to clone area, %#lx\n", clone);
			delete_area(reply.area);
			return B_ERROR;
		}
	
		for (int32 i = 0; i < reply.count; i++) {
			m_items.AddItem(new BString(items + i * B_MEDIA_NAME_LENGTH));
		}
	
		delete_area(clone);
		delete_area(reply.area);
	}
	
	m_cur_type = BString(type);
	m_item_index = 0;

	return B_OK;
}


/* virtual */ status_t
BMediaFiles::GetNextRef(BString *out_type,
						entry_ref *out_ref)
{
	CALLED();
	if(m_item_index < 0 || m_item_index >= m_items.CountItems()) {
		m_item_index = -1;
		return B_BAD_INDEX;
	}
	
	*out_type = *(BString*)m_items.ItemAt(m_item_index);
	m_item_index++;
	
	GetRefFor(m_cur_type.String(), out_type->String(), out_ref);

	return B_OK;
}


/* virtual */ status_t
BMediaFiles::GetRefFor(const char *type,
					   const char *item,
					   entry_ref *out_ref)
{
	CALLED();
	status_t rv;
	server_getreffor_request request;
	server_getreffor_reply reply;
	
	strncpy(request.type, type, B_MEDIA_NAME_LENGTH);
	strncpy(request.item, item, B_MEDIA_NAME_LENGTH);
	
	TRACE("BMediaFiles::GetRefFor: sending SERVER_GETREFFOR\n");
	
	rv = QueryServer(SERVER_GETREFFOR, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK) {
		entry_ref ref;
		*out_ref = ref;
		ERROR("BMediaFiles::GetRefFor: failed to getreffor (error %#lx)\n", rv);
		return rv;
	}

	*out_ref = reply.ref;

	return B_OK;
}


status_t
BMediaFiles::GetAudioGainFor(const char * type,
							 const char * item,
							 float * out_audio_gain)
{
	UNIMPLEMENTED();
	*out_audio_gain = 1.0f;
	return B_OK;
}


/* virtual */ status_t
BMediaFiles::SetRefFor(const char *type,
					   const char *item,
					   const entry_ref &ref)
{
	CALLED();
	status_t rv;
	server_setreffor_request request;
	server_setreffor_reply reply;
	
	strncpy(request.type, type, B_MEDIA_NAME_LENGTH);
	strncpy(request.item, item, B_MEDIA_NAME_LENGTH);
	request.ref = ref;
	
	TRACE("BMediaFiles::SetRefFor: sending SERVER_SETREFFOR\n");
	
	rv = QueryServer(SERVER_SETREFFOR, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK) {
		ERROR("BMediaFiles::SetRefFor: failed to setreffor (error %#lx)\n", rv);
		return rv;
	}

	return B_OK;
}


status_t
BMediaFiles::SetAudioGainFor(const char * type,
							 const char * item,
							 float audio_gain)
{
	UNIMPLEMENTED();
	return B_OK;
}


/* virtual */ status_t
BMediaFiles::RemoveRefFor(const char *type,
						  const char *item,
						  const entry_ref &ref)
{
	status_t rv;
	server_removereffor_request request;
	server_removereffor_reply reply;
	
	strncpy(request.type, type, B_MEDIA_NAME_LENGTH);
	strncpy(request.item, item, B_MEDIA_NAME_LENGTH);
	request.ref = ref;
	
	TRACE("BMediaFiles::RemoveRefFor: sending SERVER_REMOVEREFFOR\n");
	
	rv = QueryServer(SERVER_REMOVEREFFOR, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK) {
		ERROR("BMediaFiles::RemoveRefFor: failed to removereffor (error %#lx)\n", rv);
		return rv;
	}

	return B_OK;
}


/* virtual */ status_t
BMediaFiles::RemoveItem(const char *type,
						const char *item)
{
	status_t rv;
	server_removeitem_request request;
	server_removeitem_reply reply;
	
	strncpy(request.type, type, B_MEDIA_NAME_LENGTH);
	strncpy(request.item, item, B_MEDIA_NAME_LENGTH);
	
	TRACE("BMediaFiles::RemoveItem: sending SERVER_REMOVEITEM\n");
	
	rv = QueryServer(SERVER_REMOVEITEM, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK) {
		ERROR("BMediaFiles::RemoveItem: failed to removeitem (error %#lx)\n", rv);
		return rv;
	}

	return B_OK;
}

/*************************************************************
 * private BMediaFiles
 *************************************************************/

status_t BMediaFiles::_Reserved_MediaFiles_0(void *,...) { return B_ERROR; }
status_t BMediaFiles::_Reserved_MediaFiles_1(void *,...) { return B_ERROR; }
status_t BMediaFiles::_Reserved_MediaFiles_2(void *,...) { return B_ERROR; }
status_t BMediaFiles::_Reserved_MediaFiles_3(void *,...) { return B_ERROR; }
status_t BMediaFiles::_Reserved_MediaFiles_4(void *,...) { return B_ERROR; }
status_t BMediaFiles::_Reserved_MediaFiles_5(void *,...) { return B_ERROR; }
status_t BMediaFiles::_Reserved_MediaFiles_6(void *,...) { return B_ERROR; }
status_t BMediaFiles::_Reserved_MediaFiles_7(void *,...) { return B_ERROR; }

