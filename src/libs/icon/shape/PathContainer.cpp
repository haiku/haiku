/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "PathContainer.h"

#include <stdio.h>
#include <string.h>

#include <OS.h>

#include "VectorPath.h"

#ifdef ICON_O_MATIC
PathContainerListener::PathContainerListener() {}
PathContainerListener::~PathContainerListener() {}
#endif


// constructor
PathContainer::PathContainer(bool ownsPaths)
	: fPaths(16),
	  fOwnsPaths(ownsPaths)
#ifdef ICON_O_MATIC
	, fListeners(2)
#endif
{
}

// destructor
PathContainer::~PathContainer()
{
#ifdef ICON_O_MATIC
	int32 count = fListeners.CountItems();
	if (count > 0) {
		debugger("~PathContainer() - there are still"
				 "listeners attached\n");
	}
#endif // ICON_O_MATIC
	_MakeEmpty();
}

// #pragma mark -

// AddPath
bool
PathContainer::AddPath(VectorPath* path)
{
	return AddPath(path, CountPaths());
}

// AddPath
bool
PathContainer::AddPath(VectorPath* path, int32 index)
{
	if (!path)
		return false;

	// prevent adding the same path twice
	if (HasPath(path))
		return false;

	if (fPaths.AddItem((void*)path, index)) {
#ifdef ICON_O_MATIC
		_NotifyPathAdded(path, index);
#endif
		return true;
	}

	fprintf(stderr, "PathContainer::AddPath() - out of memory!\n");
	return false;
}

// RemovePath
bool
PathContainer::RemovePath(VectorPath* path)
{
	if (fPaths.RemoveItem((void*)path)) {
#ifdef ICON_O_MATIC
		_NotifyPathRemoved(path);
#endif
		return true;
	}

	return false;
}

// RemovePath
VectorPath*
PathContainer::RemovePath(int32 index)
{
	VectorPath* path = (VectorPath*)fPaths.RemoveItem(index);
#ifdef ICON_O_MATIC
	if (path) {
		_NotifyPathRemoved(path);
	}
#endif

	return path;
}

// MakeEmpty
void
PathContainer::MakeEmpty()
{
	_MakeEmpty();
}

// #pragma mark -

// CountPaths
int32
PathContainer::CountPaths() const
{
	return fPaths.CountItems();
}

// HasPath
bool
PathContainer::HasPath(VectorPath* path) const
{
	return fPaths.HasItem((void*)path);
}

// IndexOf
int32
PathContainer::IndexOf(VectorPath* path) const
{
	return fPaths.IndexOf((void*)path);
}

// PathAt
VectorPath*
PathContainer::PathAt(int32 index) const
{
	return (VectorPath*)fPaths.ItemAt(index);
}

// PathAtFast
VectorPath*
PathContainer::PathAtFast(int32 index) const
{
	return (VectorPath*)fPaths.ItemAtFast(index);
}

// #pragma mark -

#ifdef ICON_O_MATIC
// AddListener
bool
PathContainer::AddListener(PathContainerListener* listener)
{
	if (listener && !fListeners.HasItem((void*)listener))
		return fListeners.AddItem(listener);
	return false;
}

// RemoveListener
bool
PathContainer::RemoveListener(PathContainerListener* listener)
{
	return fListeners.RemoveItem(listener);
}
#endif // ICON_O_MATIC

// #pragma mark -

// _MakeEmpty
void
PathContainer::_MakeEmpty()
{
	int32 count = CountPaths();
	for (int32 i = 0; i < count; i++) {
		VectorPath* path = PathAtFast(i);
#ifdef ICON_O_MATIC
		_NotifyPathRemoved(path);
		if (fOwnsPaths)
			path->Release();
#else
		if (fOwnsPaths)
			delete path;
#endif
	}
	fPaths.MakeEmpty();
}

// #pragma mark -

#ifdef ICON_O_MATIC
// _NotifyPathAdded
void
PathContainer::_NotifyPathAdded(VectorPath* path, int32 index) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		PathContainerListener* listener
			= (PathContainerListener*)listeners.ItemAtFast(i);
		listener->PathAdded(path, index);
	}
}

// _NotifyPathRemoved
void
PathContainer::_NotifyPathRemoved(VectorPath* path) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		PathContainerListener* listener
			= (PathContainerListener*)listeners.ItemAtFast(i);
		listener->PathRemoved(path);
	}
}
#endif // ICON_O_MATIC

