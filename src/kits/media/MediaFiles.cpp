/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaFiles.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaFiles.h>
#include "debug.h"

/*************************************************************
 * public BMediaFiles
 *************************************************************/

const char BMediaFiles::B_SOUNDS[] = "XXX fixme";	/* for "types" */

BMediaFiles::BMediaFiles()
{
	UNIMPLEMENTED();
}


/* virtual */ 
BMediaFiles::~BMediaFiles()
{
	UNIMPLEMENTED();
}


/* virtual */ status_t
BMediaFiles::RewindTypes()
{
	UNIMPLEMENTED();

	return B_OK;
}


/* virtual */ status_t
BMediaFiles::GetNextType(BString *out_type)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


/* virtual */ status_t
BMediaFiles::RewindRefs(const char *type)
{
	UNIMPLEMENTED();

	return B_OK;
}


/* virtual */ status_t
BMediaFiles::GetNextRef(BString *out_type,
						entry_ref *out_ref)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


/* virtual */ status_t
BMediaFiles::GetRefFor(const char *type,
					   const char *item,
					   entry_ref *out_ref)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


/* virtual */ status_t
BMediaFiles::SetRefFor(const char *type,
					   const char *item,
					   const entry_ref &ref)
{
	UNIMPLEMENTED();

	return B_OK;
}


/* virtual */ status_t
BMediaFiles::RemoveRefFor(const char *type,
						  const char *item,
						  const entry_ref &ref)
{
	UNIMPLEMENTED();

	return B_OK;
}


/* virtual */ status_t
BMediaFiles::RemoveItem(const char *type,
						const char *item)
{
	UNIMPLEMENTED();

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

