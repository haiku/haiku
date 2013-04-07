/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef PACKAGE_DAEMON_H
#define PACKAGE_DAEMON_H


#include <fs_info.h>
#include <Node.h>
#include <ObjectList.h>
#include <VolumeRoster.h>

#include <Server.h>


class Root;
class Volume;


class PackageDaemon : public BServer {
public:
								PackageDaemon(status_t* _error);
	virtual						~PackageDaemon();

			status_t			Init();

	virtual	void				MessageReceived(BMessage* message);

private:
			typedef BObjectList<Root> RootList;

private:
			status_t			_RegisterVolume(dev_t device);
			void				_UnregisterVolume(Volume* volume);

			status_t			_GetOrCreateRoot(const node_ref& nodeRef,
									Root*& _root);
			Root*				_FindRoot(const node_ref& nodeRef) const;
			void				_PutRoot(Root* root);

			Volume*				_FindVolume(dev_t deviceID) const;

			void				_HandleVolumeMounted(const BMessage* message);
			void				_HandleVolumeUnmounted(const BMessage* message);

private:
			Root*				fSystemRoot;
			RootList			fRoots;
			BVolumeRoster		fVolumeWatcher;
};



#endif	// PACKAGE_DAEMON_H
