//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Mime.cpp
	Mime type C functions implementation.
*/

#include <Mime.h>

enum {
	NOT_IMPLEMENTED	= B_ERROR,
};

// update_mime_info
/*!	\brief Updates the MIME information for one or more files.
	If \a path points to a file, the MIME information for this file are
	updated only. If it points to a directory and \a recursive is non-null,
	the information for all the files in the given directory tree are updated.
	If path is \c NULL all files are considered; \a recursive is ignored in
	this case.
	\param path The path to a file or directory, or \c NULL.
	\param recursive Non-null to trigger recursive behavior.
	\param synchronous If non-null update_mime_info() waits until the
		   operation is finished, otherwise it returns immediately and the
		   update is done asynchronously.
	\param force If non-null, also the information for files are updated that
		   have already been updated.
	\return
	- \c B_OK: Everything went fine.
	- An error code otherwise.
*/
int
update_mime_info(const char *path, int recursive, int synchronous, int force)
{
	return NOT_IMPLEMENTED;
}

// create_app_meta_mime
/*!	Creates a MIME database entry for one or more applications.
	\a path should either point to an application file or should be \c NULL.
	In the first case a MIME database entry for that application is created,
	in the second case entries for all applications are created.
	\param path The path to an application file, or \c NULL.
	\param recursive Currently unused.
	\param synchronous If non-null create_app_meta_mime() waits until the
		   operation is finished, otherwise it returns immediately and the
		   operation is done asynchronously.
	\param force If non-null, entries are created even if they do already
		   exist.
	\return
	- \c B_OK: Everything went fine.
	- An error code otherwise.
*/
status_t
create_app_meta_mime(const char *path, int recursive, int synchronous,
					 int force)
{
	return NOT_IMPLEMENTED;
}

// get_device_icon
/*!	Retrieves an icon associated with a given device.
	\param dev The path to the device.
	\param icon A pointer to a buffer the icon data shall be written to.
	\param size The size of the icon. Currently the sizes 16 (small) and
		   32 (large) are supported.
	\return
	- \c B_OK: Everything went fine.
	- An error code otherwise.
*/
status_t
get_device_icon(const char *dev, void *icon, int32 size)
{
	return NOT_IMPLEMENTED;
}

