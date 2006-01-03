/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 */


#include "RegistrarThreadManager.h"

#include <RegistrarDefs.h>
#include <RegistrarThread.h>

#include <stdio.h>

//#define DBG(x) x
#define DBG(x)
#define OUT printf

using namespace BPrivate;

/*!
	\class RegistrarThreadManager
	\brief RegistrarThreadManager is the master of all threads spawned by the registrar

*/

//! Creates a new RegistrarThreadManager object
RegistrarThreadManager::RegistrarThreadManager()
	: fThreadCount(0)
{
}

// destructor
/*! \brief Destroys the RegistrarThreadManager object, killing and deleting any still
	running threads.
*/
RegistrarThreadManager::~RegistrarThreadManager()
{
	KillThreads();
}

// MessageReceived
/*! \brief Handles \c B_REG_MIME_UPDATE_THREAD_FINISHED messages, passing on all others.

	Each \c B_REG_MIME_UPDATE_THREAD_FINISHED message triggers a call to CleanupThreads().
*/
void
RegistrarThreadManager::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_REG_MIME_UPDATE_THREAD_FINISHED:
		{
			CleanupThreads();
			break;
		}
		
		default:
		{
			BHandler::MessageReceived(message);
			break;
		}
	}
}

// LaunchThread
/*! \brief Launches the given thread, passing responsiblity for it onto the
	RegistrarThreadManager object.

	\param thread Pointer to a newly allocated \c RegistrarThread object.
	
	If the result of the function is \c B_OK, the \c RegistrarThreadManager object
	assumes ownership of \a thread; if the result is an error code, it
	does not.
	
	\return
	- \c B_OK: success
	- \c B_NO_MORE_THREADS: the number of concurrently allowed threads (defined by
	                        RegistrarThreadManager::kThreadLimit) has already been reached
	- \c B_BAD_THREAD_STATE: the thread has already been launched
	- other error code: failure
*/
status_t
RegistrarThreadManager::LaunchThread(RegistrarThread *thread)
{
	status_t err = thread ? B_OK : B_BAD_VALUE;
	if (!err)
		err = fThreadCount < kThreadLimit ? (status_t)B_OK : (status_t)B_NO_MORE_THREADS;

	if (!err) {
		fThreads.push_back(thread);
		fThreadCount++;
		err = thread->Run();
		if (err) {
			std::list<RegistrarThread*>::iterator i;
			for (i = fThreads.begin(); i != fThreads.end(); i++) {
				if ((*i) == thread) {
					fThreads.erase(i);
					fThreadCount--;
					break;
				}
			}
		}
	}
	if (!err)
		DBG(OUT("RegistrarThreadManager::LaunchThread(): launched new '%s' thread, "
			"id %ld\n", thread->Name(), thread->Id()));
	return err;
}

// CleanupThreads
/*! \brief Frees the resources of any threads that are no longer running
	
	\todo This function should perhaps be triggered periodically by a
	BMessageRunner once we have our own BMessageRunner implementation.
*/
status_t
RegistrarThreadManager::CleanupThreads()
{
	std::list<RegistrarThread*>::iterator i;
	for (i = fThreads.begin(); i != fThreads.end(); ) {
		std::list<RegistrarThread*>::iterator next = i;
		next++;

		if (*i) {
			if ((*i)->IsFinished()) {
				DBG(OUT("RegistrarThreadManager::CleanupThreads(): Cleaning up thread %ld\n",
					(*i)->Id()));
				RemoveThread(i);
			}
		} else {
			OUT("WARNING: RegistrarThreadManager::CleanupThreads(): NULL mime_update_thread_shared_data "
				"pointer found in and removed from RegistrarThreadManager::fThreads list\n");
			i = fThreads.erase(i);
		}

		i = next;	
	}			
	return B_OK;
}

// ShutdownThreads
/*! \brief Requests that any running threads quit and frees the resources
	of any threads that are no longer running.

	\todo This function should be called during system shutdown around
	the same time as global B_QUIT_REQUESTED messages are sent out.
*/
status_t
RegistrarThreadManager::ShutdownThreads()
{
	std::list<RegistrarThread*>::iterator i;
	for (i = fThreads.begin(); i != fThreads.end(); ) {
		std::list<RegistrarThread*>::iterator next = i;
		next++;

		if (*i) {
			if ((*i)->IsFinished()) {
				DBG(OUT("RegistrarThreadManager::ShutdownThreads(): Cleaning up thread %ld\n",
					(*i)->Id()));
				RemoveThread(i);
			} else {
				DBG(OUT("RegistrarThreadManager::ShutdownThreads(): Shutting down thread %ld\n",
					(*i)->Id()));
				(*i)->AskToExit();
			}
		} else {
			OUT("WARNING: RegistrarThreadManager::ShutdownThreads(): NULL mime_update_thread_shared_data "
				"pointer found in and removed from RegistrarThreadManager::fThreads list\n");
			i = fThreads.erase(i);
		}

		i = next;
	}
	
	/*! \todo We may want to iterate back through the list at this point,
		snooze for a moment if find an unfinished thread, and kill it if
		it still isn't finished by the time we're done snoozing.
	*/
	
	return B_OK;
}

// KillThreads
/*! \brief Kills any running threads and frees their resources.

	\todo This function should probably be called during system shutdown
	just before kernel shutdown begins.
*/
status_t
RegistrarThreadManager::KillThreads()
{
	std::list<RegistrarThread*>::iterator i;
	for (i = fThreads.begin(); i != fThreads.end(); ) {
		std::list<RegistrarThread*>::iterator next = i;
		next++;

		if (*i) {
			if (!(*i)->IsFinished()) {
				DBG(OUT("RegistrarThreadManager::KillThreads(): Killing thread %ld\n",
					(*i)->Id()));
				status_t err = kill_thread((*i)->Id());
				if (err)
					OUT("WARNING: RegistrarThreadManager::KillThreads(): kill_thread(%ld) failed with "
						"error code 0x%lx\n", (*i)->Id(), err);
			}				
			DBG(OUT("RegistrarThreadManager::KillThreads(): Cleaning up thread %ld\n",
				(*i)->Id()));
			RemoveThread(i);
		} else {
			OUT("WARNING: RegistrarThreadManager::KillThreads(): NULL mime_update_thread_shared_data "
				"pointer found in and removed from RegistrarThreadManager::fThreads list\n");
			fThreads.erase(i);
		}

		i = next;
	}
	return B_OK;
}

// ThreadCount
/*! \brief Returns the number of threads currently under management

	This is not necessarily a count of how many threads are actually running,
	as threads may remain in the thread list that are finished and waiting
	to be cleaned up.
	
	\return The number of threads currently under management
*/
uint
RegistrarThreadManager::ThreadCount() const
{
	return fThreadCount;
}

// RemoveThread
/*! \brief Deletes the given thread and removes it from the thread list.
*/
std::list<RegistrarThread*>::iterator&
RegistrarThreadManager::RemoveThread(std::list<RegistrarThread*>::iterator &i)
{
	delete *i;
	fThreadCount--;
	return (i = fThreads.erase(i));
}

