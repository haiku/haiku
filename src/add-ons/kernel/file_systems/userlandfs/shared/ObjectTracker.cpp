// ObjectTracker.h

#include <new>
#include <typeinfo>

#include <AutoLocker.h>

#include "Debug.h"
#include "ObjectTracker.h"

static char sTrackerBuffer[sizeof(ObjectTracker)];

// constructor
ObjectTrackable::ObjectTrackable()
{
	ObjectTracker* tracker = ObjectTracker::GetDefault();
	if (tracker != NULL)
		tracker->AddTrackable(this);
}

// destructor
ObjectTrackable::~ObjectTrackable()
{
	ObjectTracker* tracker = ObjectTracker::GetDefault();
	if (tracker != NULL)
		tracker->RemoveTrackable(this);
}


// #pragma mark -

// constructor
ObjectTracker::ObjectTracker()
	: fLock("object tracker"),
	  fTrackables()
{
}

// destructor
ObjectTracker::~ObjectTracker()
{
	ObjectTrackable* trackable = fTrackables.First();
	if (trackable) {
		WARN(("ObjectTracker: WARNING: There are still undeleted objects:\n"));
#ifndef _KERNEL_MODE
		for (; trackable; trackable = fTrackables.GetNext(trackable)) {
			WARN(("  trackable: %p: type: `%s'\n", trackable,
				typeid(*trackable).name()));
		}
#endif
	}
}

// InitDefault
ObjectTracker*
ObjectTracker::InitDefault()
{
	if (!sTracker)
		sTracker = new(sTrackerBuffer) ObjectTracker;
	return sTracker;
}

// ExitDefault
void
ObjectTracker::ExitDefault()
{
	if (sTracker) {
		sTracker->~ObjectTracker();
		sTracker = NULL;
	}
}

// GetDefault
ObjectTracker*
ObjectTracker::GetDefault()
{
	return sTracker;
}

// AddTrackable
void
ObjectTracker::AddTrackable(ObjectTrackable* trackable)
{
	if (trackable) {
		AutoLocker<Locker> _(fLock);
		fTrackables.Insert(trackable);
	}
}

// RemoveTrackable
void
ObjectTracker::RemoveTrackable(ObjectTrackable* trackable)
{
	if (trackable) {
		AutoLocker<Locker> _(fLock);
		fTrackables.Remove(trackable);
	}
}

// sTracker
ObjectTracker* ObjectTracker::sTracker = NULL;

