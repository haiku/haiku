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
#include <Bitmap.h>
#include <Drivers.h>
#include <Entry.h>
#include <Mime.h>
#include <MimeType.h>
#include <mime/database_access.h>
#include <Node.h>
#include <RegistrarDefs.h>
#include <Roster.h>
#include <RosterPrivate.h>

#include <unistd.h>
#include <sys/ioctl.h>

using namespace BPrivate;

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
			err = msg.AddInt32("force", force);
		if (!err) 
			err = BRoster::Private().SendTo(&msg, &reply, true);
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
	\param force Specifies how to handle files that already have MIME
		   information:
			- \c B_UPDATE_MIME_INFO_NO_FORCE: Files that already have a
			  \c BEOS:TYPE attribute won't be updated.
			- \c B_UPDATE_MIME_INFO_FORCE_KEEP_TYPE: Files that already have a
			  \c BEOS:TYPE attribute will be updated too, but \c BEOS:TYPE
			  itself will remain untouched.
			- \c B_UPDATE_MIME_INFO_FORCE_UPDATE_ALL: Similar to
			  \c B_UPDATE_MIME_INFO_FORCE_KEEP_TYPE, but the \c BEOS:TYPE
			  attribute will be updated too.
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

// get_device_icon
/*!	Retrieves an icon associated with a given device.
	\param dev The path to the device.
	\param icon A pointer to a pre-allocated BBitmap of the correct dimension
		   to store the requested icon (16x16 for the mini and 32x32 for the
		   large icon).
	\param which Specifies the size of the icon to be retrieved:
		   \c B_MINI_ICON for the mini and \c B_LARGE_ICON for the large icon.
	            
	\todo The mounted directories for volumes can also have META:X:STD_ICON
		  attributes. Should those attributes override the icon returned
		  by ioctl(,B_GET_ICON,)?
		  
	\return
	- \c B_OK: Everything went fine.
	- An error code otherwise.
*/
status_t
get_device_icon(const char *dev, BBitmap *icon, icon_size which)
{
	// check parameters
	status_t error = (dev && icon ? B_OK : B_BAD_VALUE);
	BRect rect;
	if (error == B_OK) {
		if (which == B_MINI_ICON)
			rect.Set(0, 0, 15, 15);
		else if (which == B_LARGE_ICON)
			rect.Set(0, 0, 31, 31);
		else
			error = B_BAD_VALUE;
	}
	// check whether icon size and bitmap dimensions do match
	if (error == B_OK
		&& (icon->Bounds() != rect || icon->ColorSpace() != B_CMAP8)) {
		error = B_BAD_VALUE;
	}
	// get the icon
	if (error == B_OK)
		error = get_device_icon(dev, icon->Bits(), which);
	return error;
}

