/*
 * Copyright 2002-2008, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 */

/*!
	\file Mime.cpp
	Mime type C functions implementation.
*/

#include <mime/database_access.h>
#include <mime/UpdateMimeInfoThread.h>

using namespace BPrivate;

// update_mime_info
/*!	\brief Updates the MIME information (i.e MIME type) for one or more files.
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
	if (!path)
		return B_BAD_VALUE;

	entry_ref ref;
	status_t error = get_ref_for_path(path, &ref);
	if (error != B_OK)
		return error;

	BPrivate::Storage::Mime::UpdateMimeInfoThread updater("MIME update thread",
		B_NORMAL_PRIORITY, BMessenger(), &ref, recursive, force, NULL);
	return updater.DoUpdate();
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
	// We don't have a MIME DB...
	return B_OK;
}
