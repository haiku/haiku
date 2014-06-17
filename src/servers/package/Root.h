/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef ROOT_H
#define ROOT_H


#include <Locker.h>
#include <Node.h>
#include <ObjectList.h>
#include <OS.h>
#include <package/PackageDefs.h>
#include <String.h>

#include <Referenceable.h>

#include <package/packagefs.h>

#include "JobQueue.h"
#include "Volume.h"


class Root : public BReferenceable, private Volume::Listener {
public:
								Root();
	virtual						~Root();

			status_t			Init(const node_ref& nodeRef,
									bool isSystemRoot);

			const node_ref&		NodeRef() const		{ return fNodeRef; }
			dev_t				DeviceID() const	{ return fNodeRef.device; }
			ino_t				NodeID() const		{ return fNodeRef.node; }
			const BString&		Path() const		{ return fPath; }

			bool				IsSystemRoot() const
									{ return fIsSystemRoot; }

			status_t			RegisterVolume(Volume* volume);
			void				UnregisterVolume(Volume* volume);
									// deletes the volume (eventually)

			Volume*				FindVolume(dev_t deviceID) const;
			Volume*				GetVolume(
									BPackageInstallationLocation location);

			void				HandleRequest(BMessage* message);

private:
	// Volume::Listener
	virtual	void				VolumeNodeMonitorEventOccurred(Volume* volume);

protected:
	virtual	void				LastReferenceReleased();

private:
			struct AbstractVolumeJob;
			struct VolumeJob;
			struct ProcessNodeMonitorEventsJob;
			struct CommitTransactionJob;
			struct VolumeJobFilter;

			friend struct CommitTransactionJob;

private:
			Volume**			_GetVolume(PackageFSMountType mountType);
			Volume*				_NextVolumeFor(Volume* volume);

			void				_InitPackages(Volume* volume);
			void				_DeleteVolume(Volume* volume);
			void				_ProcessNodeMonitorEvents(Volume* volume);
			void				_CommitTransaction(Volume* volume,
									BMessage* message);

			status_t			_QueueJob(Job* job);

	static	status_t			_JobRunnerEntry(void* data);
			status_t			_JobRunner();

	static	void				_ShowError(const char* errorMessage);

private:
	mutable	BLocker				fLock;
			node_ref			fNodeRef;
			bool				fIsSystemRoot;
			BString				fPath;
			Volume*				fSystemVolume;
			Volume*				fHomeVolume;
			JobQueue			fJobQueue;
			thread_id			fJobRunner;
};


#endif	// ROOT_H
