//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <errno.h>
#include <fcntl.h>
#include <new.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <disk_scanner.h>

#include "RDiskDevice.h"
#include "RDiskDeviceList.h"
#include "RPartition.h"
#include "RSession.h"

// constructor
RDiskDevice::RDiskDevice()
	: fSessions(10, true),
	  fDeviceList(NULL),
	  fID(-1),
	  fChangeCounter(),
	  fTouched(false),
	  fPath(),
	  fFD(-1),
	  fMediaStatus(B_ERROR)
{
}

// destructor
RDiskDevice::~RDiskDevice()
{
	Unset();
}

// SetTo
status_t
RDiskDevice::SetTo(const char *path, int fd, const device_geometry *geometry,
				   status_t mediaStatus)
{
PRINT(("RDiskDevice::SetTo()\n"));
	Unset();
	status_t error = B_OK;
	fID = _NextID();
	fChangeCounter.Reset();
	fTouched = true;
	fPath.SetTo(path);
	fFD = fd;
	fGeometry = *geometry;
	fMediaStatus = mediaStatus;
	// analyze the media status
	switch (fMediaStatus) {
		case B_NO_ERROR:
		case B_DEV_NO_MEDIA:
		case B_DEV_NOT_READY:
		case B_DEV_MEDIA_CHANGE_REQUESTED:
		case B_DEV_DOOR_OPEN:
			break;
		case B_DEV_MEDIA_CHANGED:
			// the user was fast enough to change the media between opening
			// and requesting the media status -- media is in, so we are fine
			fMediaStatus = B_OK;
			break;
		default:
			error = fMediaStatus;
			break;
	}
	// scan the device for sessions
	error = _RescanSessions(B_DEVICE_CAUSE_UNKNOWN);
	// cleanup on error
	if (error != B_OK)
		Unset();
RETURN_ERROR(error);
//	return error;
}

// Unset
void
RDiskDevice::Unset()
{
	for (int32 i = CountSessions() - 1; i >= 0; i--)
		RemoveSession(i, B_DEVICE_CAUSE_UNKNOWN);
	fID = -1;
	fPath.SetTo("");
	if (fFD >= 0) {
		close(fFD);
		fFD = -1;
	}
}

// MediaChanged
status_t
RDiskDevice::MediaChanged()
{
PRINT(("RDiskDevice::MediaChanged()\n"));
	Changed();
	status_t error = B_OK;
	// get the new media status
	status_t mediaStatus;
	if (ioctl(fFD, B_GET_MEDIA_STATUS, &mediaStatus) == 0) {
		fMediaStatus = mediaStatus;
		// analyze the media status
		switch (fMediaStatus) {
			case B_NO_ERROR:
			case B_DEV_NO_MEDIA:
			case B_DEV_NOT_READY:
			case B_DEV_MEDIA_CHANGE_REQUESTED:
			case B_DEV_DOOR_OPEN:
				break;
			case B_DEV_MEDIA_CHANGED:
				// Ignore changes between the ioctl() and the one before;
				// we rescan the sessions anyway.
				fMediaStatus = B_OK;
				break;
			default:
				error = fMediaStatus;
				break;
		}
	}
	// get the device geometry
	if (ioctl(fFD, B_GET_GEOMETRY, &fGeometry) != 0)
		error = errno;
	// rescan sessions
	if (error == B_OK)
		error = _RescanSessions(B_DEVICE_CAUSE_PARENT_CHANGED);
	// notification
	if (fDeviceList)
		fDeviceList->MediaChanged(this, B_DEVICE_CAUSE_MEDIA_CHANGED);
	return error;
}

// SessionLayoutChanged
status_t
RDiskDevice::SessionLayoutChanged()
{
PRINT(("RDiskDevice::SessionLayoutChanged()\n"));
	Changed();
	status_t error = B_OK;
	error = _RescanSessions(B_DEVICE_CAUSE_UNKNOWN);
	return error;
}

// Size
off_t
RDiskDevice::Size() const
{
	return (off_t)fGeometry.bytes_per_sector * fGeometry.sectors_per_track
			* fGeometry.cylinder_count * fGeometry.head_count;
}

// AddSession
status_t
RDiskDevice::AddSession(const session_info *sessionInfo, uint32 cause)
{
	status_t error = B_OK;
	if (RSession *session = new(nothrow) RSession) {
		error = session->SetTo(fFD, sessionInfo);
		if (error == B_OK)
			AddSession(session, cause);
		else
			delete session;
	} else
		error = B_NO_MEMORY;
	return error;
}

// AddSession
bool
RDiskDevice::AddSession(RSession *session, uint32 cause)
{
	bool success = false;
	if (session) {
		success = fSessions.AddItem(session);
		if (success) {
			session->SetDevice(this);
			Changed();
			if (RDiskDeviceList *deviceList = DeviceList())
				deviceList->SessionAdded(session, cause);
		}
	}
	return success;
}

// RemoveSession
bool
RDiskDevice::RemoveSession(int32 index, uint32 cause)
{
	RSession *session = SessionAt(index);
	if (session) {
		Changed();
		if (RDiskDeviceList *deviceList = DeviceList())
			deviceList->SessionRemoved(session, cause);
		session->SetDevice(NULL);
		fSessions.RemoveItemAt(index);
		delete session;
	}
	return (session != NULL);
}

// RemoveSession
bool
RDiskDevice::RemoveSession(RSession *session, uint32 cause)
{
	bool success = false;
	if (session) {
		int32 index = fSessions.IndexOf(session);
		if (index >= 0)
			success = RemoveSession(index, cause);
	}
	return success;
}

// Update
status_t
RDiskDevice::Update()
{
	RChangeCounter::Locker lock(fChangeCounter);
	status_t error = B_OK;
	status_t mediaStatus = B_OK;
	if (ioctl(fFD, B_GET_MEDIA_STATUS, &mediaStatus) == 0) {
		if (mediaStatus == B_DEV_MEDIA_CHANGED
			|| (mediaStatus == B_NO_ERROR) != (fMediaStatus == B_NO_ERROR)) {
			// The media status is B_DEV_MEDIA_CHANGED or it changed from
			// B_NO_ERROR to some error code or the other way around.
			error = MediaChanged();
		} else {
			// TODO: notifications?
			fMediaStatus = mediaStatus;
			// The media has not been changed. If this is a read-only device,
			// then we are safe, since nothing can have changed. Otherwise
			// we check the sessions.
			if (!IsReadOnly() && fMediaStatus == B_OK) {
				session_info sessionInfo;
				for (int32 i = 0; error == B_OK; i++) {
					// get the session info
					status_t status = get_nth_session_info(fFD, i,
														   &sessionInfo);
					if (status != B_OK) {
						if (status == B_ENTRY_NOT_FOUND) {
							// remove disappeared sessions
							for (int32 k = CountSessions() - 1; k >= i; k--)
								RemoveSession(k, B_DEVICE_CAUSE_UNKNOWN);
						} else {
							// ignore errors -- we can't help it
//							error = status;
						}
						break;
					}
					// check the session
					if (RSession *session = SessionAt(i)) {
						if (session->Offset() == sessionInfo.offset
							&& session->Size() == sessionInfo.size) {
							session->Update(&sessionInfo);
						} else {
							// session layout changed
							error = SessionLayoutChanged();
							break;
						}
					} else {
						// session added
						error = AddSession(&sessionInfo,
										   B_DEVICE_CAUSE_UNKNOWN);
					}
				}
			}
		}
	}
	return error;
}

// Archive
status_t
RDiskDevice::Archive(BMessage *archive) const
{
	status_t error = (archive ? B_OK : B_BAD_VALUE);
	// ID and change counter
	if (error == B_OK)
		error = archive->AddInt32("id", fID);
	if (error == B_OK)
		error = archive->AddInt32("change_counter", ChangeCounter());
	// geometry
	if (error == B_OK)
		error = archive->AddInt64("size", Size());
	if (error == B_OK)
		error = archive->AddInt32("block_size", BlockSize());
	if (error == B_OK)
		error = archive->AddInt8("type", (int8)fGeometry.device_type);
	if (error == B_OK)
		error = archive->AddBool("removable", fGeometry.removable);
	if (error == B_OK)
		error = archive->AddBool("read_only", fGeometry.read_only);
	if (error == B_OK)
		error = archive->AddBool("write_once", fGeometry.write_once);
	// other data
	if (error == B_OK)
		error = archive->AddString("path", fPath.String());
	if (error == B_OK)
		error = archive->AddInt32("media_status", fMediaStatus);
	// sessions
	for (int32 i = 0; const RSession *session = SessionAt(i); i++) {
		BMessage sessionArchive;
		error = session->Archive(&sessionArchive);
		if (error == B_OK)
			error = archive->AddMessage("sessions", &sessionArchive);
		if (error != B_OK)
			break;
	}
	return error;
}

// Dump
void
RDiskDevice::Dump() const
{
	printf("device `%s'\n", Path());
	for (int32 i = 0; RSession *session = SessionAt(i); i++)
		session->Dump();
}

// _RescanSessions
status_t
RDiskDevice::_RescanSessions(uint32 cause)
{
	status_t error = B_OK;
	// remove the current sessions
	for (int32 i = CountSessions() - 1; i >= 0; i--)
		RemoveSession(i, cause);
	// scan the device for sessions, if we have a media
	if (fMediaStatus == B_OK) {
		session_info sessionInfo;
		for (int32 i = 0; error == B_OK; i++) {
			// get the session info
			status_t status = get_nth_session_info(fFD, i, &sessionInfo);
			if (status != B_OK) {
				if (status != B_ENTRY_NOT_FOUND) {
					// ignore errors -- we can't help it
//					error = status;
				}
				break;
			}
			// create and add a RSession
			error = AddSession(&sessionInfo, cause);
		}
	}
	RETURN_ERROR(error);
}

// _NextID
int32
RDiskDevice::_NextID()
{
	return atomic_add(&fNextID, 1);
}

// fNextID
vint32 RDiskDevice::fNextID = 0;

