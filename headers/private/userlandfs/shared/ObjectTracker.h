/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_OBJECT_TRACKER_H
#define USERLAND_FS_OBJECT_TRACKER_H

#include <util/DoublyLinkedList.h>

#include "Locker.h"

namespace UserlandFSUtil {

class ObjectTracker;
class GetObjectTrackableLink;

// ObjectTrackable
class ObjectTrackable {
public:
								ObjectTrackable();
	virtual						~ObjectTrackable();

private:
	friend class ObjectTracker;
	friend class GetObjectTrackableLink;

			DoublyLinkedListLink<ObjectTrackable> fLink;
};

// GetObjectTrackableLink
struct GetObjectTrackableLink {
	inline DoublyLinkedListLink<ObjectTrackable> *operator()(
		ObjectTrackable* trackable) const
	{
		return &trackable->fLink;
	}

	inline const DoublyLinkedListLink<ObjectTrackable> *operator()(
		const ObjectTrackable* trackable) const
	{
		return &trackable->fLink;
	}
};

// ObjectTracker
class ObjectTracker {
private:
								ObjectTracker();
								~ObjectTracker();

public:

	static	ObjectTracker*		InitDefault();
	static	void				ExitDefault();
	static	ObjectTracker*		GetDefault();

private:
			friend class ObjectTrackable;

			void				AddTrackable(ObjectTrackable* trackable);
			void				RemoveTrackable(ObjectTrackable* trackable);

private:
			Locker				fLock;
			DoublyLinkedList<ObjectTrackable, GetObjectTrackableLink>
				fTrackables;

	static	ObjectTracker*		sTracker;
};

}	// namespace UserlandFSUtil

using UserlandFSUtil::ObjectTrackable;
using UserlandFSUtil::ObjectTracker;

#ifdef DEBUG_OBJECT_TRACKING
#	define ONLY_OBJECT_TRACKABLE_BASE_CLASS		: private ObjectTrackable
#	define FIRST_OBJECT_TRACKABLE_BASE_CLASS	private ObjectTrackable,
#else
#	define ONLY_OBJECT_TRACKABLE_BASE_CLASS
#	define FIRST_OBJECT_TRACKABLE_BASE_CLASS
#endif

#endif	// USERLAND_FS_OBJECT_TRACKER_H
