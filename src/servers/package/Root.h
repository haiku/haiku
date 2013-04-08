/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef ROOT_H
#define ROOT_H


#include <Node.h>
#include <ObjectList.h>
#include <OS.h>
#include <String.h>

#include <Referenceable.h>

#include <packagefs.h>

#include "JobQueue.h"
#include "Volume.h"


class Root : public BReferenceable, private Volume::Listener {
public:
								Root();
	virtual						~Root();

			status_t			Init(const node_ref& nodeRef);

			const node_ref&		NodeRef() const		{ return fNodeRef; }
			dev_t				DeviceID() const	{ return fNodeRef.device; }
			ino_t				NodeID() const		{ return fNodeRef.node; }
			const BString&		Path() const		{ return fPath; }

			status_t			RegisterVolume(Volume* volume);
			void				UnregisterVolume(Volume* volume);
									// deletes the volume (eventually)

			Volume*				FindVolume(dev_t deviceID) const;

private:
	// Volume::Listener
	virtual	void				VolumeNodeMonitorEventOccurred(Volume* volume);

protected:
	virtual	void				LastReferenceReleased();

private:
			struct VolumeJob;

private:
			Volume**			_GetVolume(PackageFSMountType mountType);

			void				_InitPackages(Volume* volume);
			void				_DeleteVolume(Volume* volume);
			void				_ProcessNodeMonitorEvents(Volume* volume);

			status_t			_QueueJob(Job* job);

	static	status_t			_JobRunnerEntry(void* data);
			status_t			_JobRunner();

private:
			node_ref			fNodeRef;
			BString				fPath;
			Volume*				fSystemVolume;
			Volume*				fCommonVolume;
			Volume*				fHomeVolume;
			JobQueue			fJobQueue;
			thread_id			fJobRunner;
};


#endif	// ROOT_H
