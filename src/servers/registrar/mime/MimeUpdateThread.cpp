/*
 * Copyright 2002-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 */

/*!
	\file MimeUpdateThread.cpp
	MimeUpdateThread implementation
*/

#include "MimeUpdateThread.h"

#include <stdio.h>

#include <Directory.h>
#include <Message.h>
#include <Path.h>
#include <RegistrarDefs.h>
#include <Volume.h>

#include <storage_support.h>

//#define DBG(x) x
#define DBG(x)
#define OUT printf

namespace BPrivate {
namespace Storage {
namespace Mime {

/*!	\class MimeUpdateThread
	\brief RegistrarThread class implementing the common functionality of
	update_mime_info() and create_app_meta_mime()
*/


/*! \brief Creates a new MimeUpdateThread object.

	If \a replyee is non-NULL and construction succeeds, the MimeThreadObject
	assumes resposibility for its deletion.
	
	Also, if \c non-NULL, \a replyee is expected to be a \c B_REG_MIME_UPDATE_MIME_INFO
	or a \c B_REG_MIME_CREATE_APP_META_MIME	message with a \c true \c "synchronous"
	field detached from the registrar's	mime manager looper (though this is not verified).
	The message will be	replied to at the end of the thread's execution.
*/
MimeUpdateThread::MimeUpdateThread(const char *name, int32 priority,
		Database *database, BMessenger managerMessenger, const entry_ref *root,
		bool recursive, int32 force, BMessage *replyee)
	:
	RegistrarThread(name, priority, managerMessenger),
	fDatabase(database),
	fRoot(root ? *root : entry_ref()),
	fRecursive(recursive),
	fForce(force),
	fReplyee(replyee),
	fStatus(root ? B_OK : B_BAD_VALUE)
{
}


/*!	\brief Destroys the MimeUpdateThread object.

	If the object was properly initialized (i.e. InitCheck() returns \c B_OK) and
	the replyee message passed to the constructor was \c non-NULL, the replyee
	message is deleted.
*/
MimeUpdateThread::~MimeUpdateThread()
{
	// delete our acquired BMessage
	if (InitCheck() == B_OK)
		delete fReplyee;
}


/*! \brief Returns the initialization status of the object
*/
status_t
MimeUpdateThread::InitCheck()
{
	status_t err = RegistrarThread::InitCheck();
	if (!err)
		err = fStatus;
	return err;
}


/*! \brief Implements the common functionality of update_mime_info() and
	create_app_meta_mime(), namely iterating through the filesystem and
	updating entries.
*/
status_t
MimeUpdateThread::ThreadFunction()
{
	status_t err = InitCheck();

	// The registrar is using this, too, so we better make sure we
	// don't run into troubles
	try {
		// Do the updates
		if (!err)
			err = UpdateEntry(&fRoot);
	} catch (...) {
		err = B_ERROR;
	}

	// Send a reply if we have a message to reply to
	if (fReplyee) {
		BMessage reply(B_REG_RESULT);
		status_t error = reply.AddInt32("result", err);
		err = error;
		if (!err)
			err = fReplyee->SendReply(&reply);
	}

	// Flag ourselves as finished
	fIsFinished = true;
	// Notify the thread manager to make a cleanup run
	if (!err) {
		BMessage msg(B_REG_MIME_UPDATE_THREAD_FINISHED);
		status_t error = fManagerMessenger.SendMessage(&msg, (BHandler*)NULL, 500000);
		if (error)
			OUT("WARNING: ThreadManager::ThreadEntryFunction(): Termination notification "
				"failed with error 0x%" B_PRIx32 "\n", error);
	}
	DBG(OUT("(id: %ld) exiting mime update thread with result 0x%" B_PRIx32 "\n",
		find_thread(NULL), err));
	return err;
}


/*! \brief Returns true if the given device supports attributes, false
	if not (or if an error occurs while determining).
	
	Device numbers and their corresponding support info are cached in
	a std::list to save unnecessarily \c statvfs()ing devices that have
	already been statvfs()ed (which might otherwise happen quite often
	for a device that did in fact support attributes).

	\return
	- \c true: The device supports attributes
	- \c false: The device does not support attributes, or there was an
	            error while determining
*/
bool
MimeUpdateThread::DeviceSupportsAttributes(dev_t device)
{
	// See if an entry for this device already exists
	std::list< std::pair<dev_t,bool> >::iterator i;
	for (i = fAttributeSupportList.begin();
		   i != fAttributeSupportList.end();
		     i++)
	{
		if (i->first == device)
			return i->second;
	}
	
	bool result = false;

	// If we get here, no such device is yet in our list,
	// so we attempt to remedy the situation
	BVolume volume;
	status_t err = volume.SetTo(device);
	if (!err) {
		result = volume.KnowsAttr();
		// devices supporting attributes are likely to be queried
		// again, devices not supporting attributes are not
		std::pair<dev_t,bool> p(device, result);
		if (result)
			fAttributeSupportList.push_front(p);
		else
			fAttributeSupportList.push_back(p);
	}
	
	return result;		
}

// UpdateEntry
/*! \brief Updates the given entry and then recursively updates all the entry's child
	entries	if the entry is a directory and \c fRecursive is true.
*/
status_t
MimeUpdateThread::UpdateEntry(const entry_ref *ref)
{
	status_t err = ref ? B_OK : B_BAD_VALUE;
	bool entryIsDir = false;
	
	// Look to see if we're being terminated
	if (!err && fShouldExit)
		err = B_CANCELED;
		
	// Before we update, make sure this entry lives on a device that supports
	// attributes. If not, we skip it and any of its children for
	// updates (we don't signal an error, however).

//BPath path(ref);
//printf("Updating '%s' (%s)... \n", path.Path(),
//	(DeviceSupportsAttributes(ref->device) ? "yes" : "no"));

	if (!err && (device_is_root_device(ref->device)
				|| DeviceSupportsAttributes(ref->device))) {	
		// Update this entry
		if (!err) {
			// R5 appears to ignore whether or not the update succeeds.
			DoMimeUpdate(ref, &entryIsDir);
		}

		// If we're recursing and this is a directory, update
		// each of the directory's children as well
		if (!err && fRecursive && entryIsDir) {		
			BDirectory dir;		
			err = dir.SetTo(ref);
			if (!err) {
				entry_ref childRef;
				while (!err) {
					err = dir.GetNextRef(&childRef);
					if (err) {
						// If we've come to the end of the directory listing,
						// it's not an error.
						if (err == B_ENTRY_NOT_FOUND)
						 	err = B_OK;
						break;
					} else {
						err = UpdateEntry(&childRef);				
					}			
				}		
			}			
		}
	}
	return err;			  
}

}	// namespace Mime
}	// namespace Storage
}	// namespace BPrivate
