/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_FILE_SYSTEM_H
#define USERLAND_FS_FILE_SYSTEM_H

#include <fs_interface.h>
#include <image.h>
#include <OS.h>

#include <kernel/util/DoublyLinkedList.h>

#include "FSCapabilities.h"
#include "Locker.h"


namespace UserlandFS {

class RequestThreadContext;
class Volume;


class FileSystem {
public:
								FileSystem(const char* fsName);
	virtual						~FileSystem();

	static	FileSystem*			GetInstance();

			const char*			GetName() const	{ return fName; }

	virtual	status_t			CreateVolume(Volume** volume, dev_t id) = 0;
	virtual	status_t			DeleteVolume(Volume* volume) = 0;

	virtual	void				InitRequestThreadContext(
									RequestThreadContext* context);

			void				RegisterVolume(Volume* volume);
			void				UnregisterVolume(Volume* volume);
			Volume*				VolumeWithID(dev_t id);

			void				GetCapabilities(
									FSCapabilities& capabilities) const
									{ capabilities = fCapabilities; }
			client_fs_type		GetClientFSType() const
									{ return fClientFSType; }

protected:
			typedef DoublyLinkedList<Volume> VolumeList;

protected:
			Locker				fLock;
			VolumeList			fVolumes;
			FSCapabilities		fCapabilities;
			client_fs_type		fClientFSType;
			char				fName[B_FILE_NAME_LENGTH];

	static	FileSystem*			sInstance;
};

}	// namespace UserlandFS

using UserlandFS::FileSystem;


// implemented by the interface implementations
extern "C" status_t userlandfs_create_file_system(const char* fsName,
	image_id image, FileSystem** _fileSystem);

#endif	// USERLAND_FS_FILE_SYSTEM_H
