//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		WatchingService.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Features everything needed to provide a watching service.
//------------------------------------------------------------------------------

#include <List.h>

#include "Watcher.h"
#include "WatchingService.h"

using namespace std;

/*!	\class WatchingService
	\brief Features everything needed to provide a watching service.

	A watcher is represented by an object of the Watcher or a derived class.
	It is identified by its target BMessenger. The service can't contain
	more than one watcher with the same target BMessenger at a time.

	New watchers can be registered with AddWatcher(), registered ones
	unregistered via RemoveRegister(). NotifyWatchers() sends a specified
	message to all registered watchers, or, if an optional WatcherFilter is
	supplied, only to those watchers it selects.
*/

/*!	\var typedef map<BMessenger,Watcher*> WatchingService::watcher_map
	\brief Watcher container type.

	Defined for convenience.
*/

/*!	\var WatchingService::watcher_map WatchingService::fWatchers
	\brief Container for the watchers registered to the service.

	For each registered watcher \code Watcher *watcher \endcode, the map
	contains an entry \code (watcher->Target(), watcher) \endcode.
*/


// constructor
/*!	\brief Creates a new watching service.

	The list of watchers is initially empty.
*/
WatchingService::WatchingService()
	: fWatchers()
{
}

// destructor
/*!	\brief Frees all resources associated with the object.

	All registered watchers are deleted.
*/
WatchingService::~WatchingService()
{
	// delete the watchers
	for (watcher_map::iterator it = fWatchers.begin();
		 it != fWatchers.end();
		 ++it) {
		delete it->second;
	}
}

// AddWatcher
/*!	\brief Registers a new watcher to the watching service.

	The ownership of \a watcher is transfered to the watching service, that
	is the caller must not delete it after the method returns.

	If the service already contains a Watcher with the same target BMessenger
	(Watcher::Target()), the old watcher is removed and deleted before the
	new one is added..

	\param watcher The watcher to be registered.
	\return \c true, if \a watcher is not \c NULL and adding was successfully,
			\c false otherwise.
*/
bool
WatchingService::AddWatcher(Watcher *watcher)
{
	bool result = (watcher);
	if (result) {
		RemoveWatcher(watcher->Target(), true);
		fWatchers[watcher->Target()] = watcher;
	}
	return result;
}

// AddWatcher
/*!	\brief Registers a new watcher to the watching service.

	A new \c Watcher is created with \a target as its target and added to
	the watching service. The caller retains ownership of \a target, but the
	newly created Watcher is owned by the watching service.

	If the service already contains a Watcher with the same target BMessenger
	(Watcher::Target()), the old watcher is removed and deleted before the
	new one is added..

	\param target The target BMessenger a Watcher shall be registered for.
	\return \c true, if a new Watcher could be created and added successfully,
			\c false otherwise.
*/
bool
WatchingService::AddWatcher(const BMessenger &target)
{
	return AddWatcher(new(nothrow) Watcher(target));
}

// RemoveWatcher
/*!	\brief Unregisters a watcher from the watching service and optionally
		   deletes it.

	If \a deleteWatcher is \c false, the ownership of \a watcher is transfered
	to the caller, otherwise it is deleted.

	\param watcher The watcher to be unregistered.
	\param deleteWatcher If \c true, the watcher is deleted after being
		   removed.
	\return \c true, if \a watcher was not \c NULL and registered to the
			watching service before, \c false otherwise.
*/
bool
WatchingService::RemoveWatcher(Watcher *watcher, bool deleteWatcher)
{
	watcher_map::iterator it = fWatchers.find(watcher->Target());
	bool result = (it != fWatchers.end() && it->second == watcher);
	if (result) {
		if (deleteWatcher)
			delete it->second;
		fWatchers.erase(it);
	}
	return result;
}

// RemoveWatcher
/*!	\brief Unregisters a watcher from the watching service and optionally
		   deletes it.

	The watcher is identified by its target BMessenger.

	If \a deleteWatcher is \c false, the ownership of the concerned watcher is
	transfered to the caller, otherwise it is deleted.

	\param target The target BMessenger identifying the watcher to be
		   unregistered.
	\param deleteWatcher If \c true, the watcher is deleted after being
		   removed.
	\return \c true, if a watcher with the specified target was registered to
			the watching service before, \c false otherwise.
*/
bool
WatchingService::RemoveWatcher(const BMessenger &target, bool deleteWatcher)
{
	watcher_map::iterator it = fWatchers.find(target);
	bool result = (it != fWatchers.end());
	if (result) {
		if (deleteWatcher)
			delete it->second;
		fWatchers.erase(it);
	}
	return result;
}

// NotifyWatchers
/*!	\brief Sends a notification message to all watcher targets selected by a
		   supplied filter.

	If no filter is supplied the message is sent to all watchers. Otherwise
	each watcher (and the notification message) is passed to its
	WatcherFilter::Filter() method and the message is sent only to those
	watchers for which \c true is returned.

	If a sending a message to a watcher's target failed, because it became
	invalid, the watcher is unregistered and deleted.

	\param message The message to be sent to the watcher targets.
	\param filter The filter selecting the watchers to which the message
		   is be sent. May be \c NULL.
*/
void
WatchingService::NotifyWatchers(BMessage *message, WatcherFilter *filter)
{
	if (message) {
		BList staleWatchers;
		// deliver the message
		for (watcher_map::iterator it = fWatchers.begin();
			 it != fWatchers.end();
			 ++it) {
			Watcher *watcher = it->second;
// TODO: If a watcher is invalid, but the filter never selects it, it will
// not be removed.
			if (!filter || filter->Filter(watcher, message)) {
				status_t error = watcher->SendMessage(message);
				if (error != B_OK && !watcher->Target().IsValid())
					staleWatchers.AddItem(watcher);
			}
		}
		// remove the stale watchers
		for (int32 i = 0;
			 Watcher *watcher = (Watcher*)staleWatchers.ItemAt(i);
			 i++) {
			RemoveWatcher(watcher, true);
		}
	}
}

