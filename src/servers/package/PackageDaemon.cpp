/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "PackageDaemon.h"

#include <errno.h>
#include <string.h>

#include <Directory.h>
#include <NodeMonitor.h>

#include <AutoDeleter.h>
#include <package/DaemonDefs.h>

#include "DebugSupport.h"
#include "Root.h"
#include "Volume.h"


using namespace BPackageKit::BPrivate;


PackageDaemon::PackageDaemon(status_t* _error)
	:
	BServer(B_PACKAGE_DAEMON_APP_SIGNATURE, false, _error),
	fSystemRoot(NULL),
	fRoots(10, true),
	fVolumeWatcher()
{
}


PackageDaemon::~PackageDaemon()
{
}


status_t
PackageDaemon::Init()
{
	status_t error = fVolumeWatcher.StartWatching(BMessenger(this, this));
	if (error != B_OK) {
		ERROR("PackageDaemon::Init(): failed to start volume watching: %s\n",
			strerror(error));
	}

	// register all packagefs volumes
	for (int32 cookie = 0;;) {
		dev_t device = next_dev(&cookie);
		if (device < 0)
			break;

		_RegisterVolume(device);
	}

	return B_OK;
}


void
PackageDaemon::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (message->FindInt32("opcode", &opcode) != B_OK)
				break;

			if (opcode == B_DEVICE_MOUNTED)
				_HandleVolumeMounted(message);
			else if (opcode == B_DEVICE_UNMOUNTED)
				_HandleVolumeUnmounted(message);
			break;
		}

		case B_MESSAGE_GET_INSTALLATION_LOCATION_INFO:
		case B_MESSAGE_COMMIT_TRANSACTION:
		{
			if (fSystemRoot != NULL)
				fSystemRoot->HandleRequest(DetachCurrentMessage());
			break;
		}

		default:
			BServer::MessageReceived(message);
			break;
	}
}


status_t
PackageDaemon::_RegisterVolume(dev_t deviceID)
{
	// get the FS info and check whether this is a package FS volume at all
	fs_info info;
	status_t error = fs_stat_dev(deviceID, &info);
	if (error != 0)
		RETURN_ERROR(error);

	if (strcmp(info.fsh_name, "packagefs") != 0)
		RETURN_ERROR(B_BAD_VALUE);

	// create a volume
	Volume* volume = new(std::nothrow) Volume(this);
	if (volume == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<Volume> volumeDeleter(volume);

	node_ref rootRef;
	error = volume->Init(node_ref(info.dev, info.root), rootRef);
	if (error != B_OK)
		RETURN_ERROR(error);

	if (volume->MountType() == PACKAGE_FS_MOUNT_TYPE_CUSTOM) {
// TODO: Or maybe not?
		INFORM("skipping custom mounted volume at \"%s\"\n",
			volume->Path().String());
		return B_OK;
	}

	// get the root for the volume and register it
	Root* root;
	error = _GetOrCreateRoot(rootRef, root);
	if (error != B_OK)
		RETURN_ERROR(error);

	error = root->RegisterVolume(volume);
	if (error != B_OK) {
		_PutRoot(root);
		RETURN_ERROR(error);
	}
	volumeDeleter.Detach();

	INFORM("volume at \"%s\" registered\n", volume->Path().String());

	return B_OK;
}


void
PackageDaemon::_UnregisterVolume(Volume* volume)
{
	volume->Unmounted();

	INFORM("volume at \"%s\" unregistered\n", volume->Path().String());

	Root* root = volume->GetRoot();
	root->UnregisterVolume(volume);

	_PutRoot(root);
}


status_t
PackageDaemon::_GetOrCreateRoot(const node_ref& nodeRef, Root*& _root)
{
	Root* root = _FindRoot(nodeRef);
	if (root != NULL) {
		root->AcquireReference();
	} else {
		root = new(std::nothrow) Root;
		if (root == NULL)
			RETURN_ERROR(B_NO_MEMORY);
		ObjectDeleter<Root> rootDeleter(root);

		bool isSystemRoot = false;
		if (fSystemRoot == NULL) {
			struct stat st;
			isSystemRoot = stat("/boot", &st) == 0
				&& node_ref(st.st_dev, st.st_ino) == nodeRef;
		}

		status_t error = root->Init(nodeRef, isSystemRoot);
		if (error != B_OK)
			RETURN_ERROR(error);

		if (!fRoots.AddItem(root))
			RETURN_ERROR(B_NO_MEMORY);

		rootDeleter.Detach();

		if (isSystemRoot)
			fSystemRoot = root;

		INFORM("root at \"%s\" (device: %" B_PRIdDEV ", node: %" B_PRIdINO ") "
			"registered\n", root->Path().String(), nodeRef.device,
			nodeRef.node);
	}

	_root = root;
	return B_OK;
}


Root*
PackageDaemon::_FindRoot(const node_ref& nodeRef) const
{
	for (int32 i = 0; Root* root = fRoots.ItemAt(i); i++) {
		if (root->NodeRef() == nodeRef)
			return root;
	}

	return NULL;
}


void
PackageDaemon::_PutRoot(Root* root)
{
	if (root->ReleaseReference() == 1) {
		INFORM("root at \"%s\" unregistered\n", root->Path().String());
		fRoots.RemoveItem(root, true);
			// deletes the object
	}
}


Volume*
PackageDaemon::_FindVolume(dev_t deviceID) const
{
	for (int32 i = 0; Root* root = fRoots.ItemAt(i); i++) {
		if (Volume* volume = root->FindVolume(deviceID))
			return volume;
	}

	return NULL;
}


void
PackageDaemon::_HandleVolumeMounted(const BMessage* message)
{
	int32 device;
	if (message->FindInt32("new device", &device) != B_OK)
		return;

	// _RegisterVolume() also checks whether it is a package FS volume, so we
	// don't need to bother.
	_RegisterVolume(device);
}


void
PackageDaemon::_HandleVolumeUnmounted(const BMessage* message)
{
	int32 device;
	if (message->FindInt32("device", &device) != B_OK)
		return;

	if (Volume* volume = _FindVolume(device))
		_UnregisterVolume(volume);
}


// #pragma mark -


int
main(int argc, const char* const* argv)
{
	status_t error;
	PackageDaemon daemon(&error);
	if (error == B_OK)
		error = daemon.Init();
	if (error != B_OK) {
		FATAL("failed to init server application: %s\n", strerror(error));
		return 1;
	}

	daemon.Run();
	return 0;
}
