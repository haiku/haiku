//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Mime.cpp
	Mime type C functions implementation.
*/

#include <fs_attr.h>
#include <fs_info.h>
#include <Drivers.h>
#include <Entry.h>
#include <Mime.h>
#include <MimeType.h>
#include <mime/database_access.h>
#include <Node.h>
#include <RegistrarDefs.h>
#include <Roster.h>

#include <unistd.h>

enum {
	NOT_IMPLEMENTED	= B_ERROR,
};

// do_mime_update
//! Helper function that contacts the registrar for mime update calls
status_t do_mime_update(int32 what, const char *path, int recursive,
	int synchronous, int force)
{
	BEntry root;
	entry_ref ref;
		
	status_t err = root.SetTo(path ? path : "/");
	if (!err)
		err = root.GetRef(&ref);
	if (!err) {
		BMessage msg(what);
		BMessage reply;
		status_t result;
		
		// Build and send the message, read the reply
		if (!err)
			err = msg.AddRef("entry", &ref);
		if (!err)
			err = msg.AddBool("recursive", recursive);
		if (!err)
			err = msg.AddBool("synchronous", synchronous);
		if (!err)
			err = msg.AddBool("force", force);
		if (!err) 
			err = _send_to_roster_(&msg, &reply, true);
		if (!err)
			err = reply.what == B_REG_RESULT ? B_OK : B_BAD_VALUE;
		if (!err)
			err = reply.FindInt32("result", &result);
		if (!err) 
			err = result;
	}
	return err;
}

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
	// Force recursion when given a NULL path
	if (!path)
		recursive = true;

	return do_mime_update(B_REG_MIME_UPDATE_MIME_INFO, path, recursive,
		synchronous, force);	
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
	// If path is NULL, we are recursive, otherwise no.
	recursive = !path;
	
	return do_mime_update(B_REG_MIME_CREATE_APP_META_MIME, path, recursive,
		synchronous, force);
}

// get_device_icon
/*!	Retrieves an icon associated with a given device.
	\param dev The path to the device.
	\param icon A pointer to a buffer the icon data shall be written to.
	\param size The size of the icon. Currently the sizes 16 (small, i.e
	            \c B_MINI_ICON) and 32 (large, 	i.e. \c B_LARGE_ICON) are
	            supported.
	            
	\todo The mounted directories for volumes can also have META:X:STD_ICON
		  attributes. Should those attributes override the icon returned
		  by ioctl(,B_GET_ICON,)?
		  
	\return
	- \c B_OK: Everything went fine.
	- An error code otherwise.
*/
status_t
get_device_icon(const char *dev, void *icon, int32 size)
{
	status_t err = dev && icon
				     && (size == B_LARGE_ICON || size == B_MINI_ICON)
				       ? B_OK : B_BAD_VALUE;
	
	int fd = -1;
	
	if (!err) {
		fd = open(dev, O_RDONLY);
		err = fd != -1 ? B_OK : B_BAD_VALUE;
	}
	if (!err) {
		device_icon iconData = { size, icon };
		err = ioctl(fd, B_GET_ICON, &iconData);
	}
	if (fd != -1) {
		// If the file descriptor was open()ed, we need to close it
		// regardless. Only if we haven't yet encountered any errors
		// do we make note close()'s return value, however.
		status_t error = close(fd);
		if (!err)
			err = error;
	}
	return err;	
}
























