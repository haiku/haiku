//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file ThreadManager.cpp
	ThreadManager implementation
*/

#include "ThreadManager.h"

#include <RegistrarDefs.h>
#include <RegistrarThread.h>

#include <stdio.h>

#define DBG(x) x
//#define DBG(x)
#define OUT printf

/*!
	\class ThreadManager
	\brief ThreadManager is the master of all threads spawned by the registrar

*/

//! Creates a new ThreadManager object
ThreadManager::ThreadManager()
	: fThreadCount(0)
{
}

// destructor
/*! \brief Destroys the ThreadManager object, killing and deleting any still
	running threads.
*/
ThreadManager::~ThreadManager()
{
	KillThreads();
}

// MessageReceived
/*! \brief Handles \c B_REG_MIME_UPDATE_THREAD_FINISHED messages, passing on all others.

	Each \c B_REG_MIME_UPDATE_THREAD_FINISHED message triggers a call to CleanupThreads().
*/
void
ThreadManager::MessageReceived(BMessage* message)
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
	ThreadManager object.

	\param thread Pointer to a newly allocated \c RegistrarThread object.
	
	If the result of the function is \c B_OK, the \c ThreadManager object
	assumes ownership of \a thread; if the result is an error code, it
	does not.
	
	\return
	- \c B_OK: success
	- \c B_NO_MORE_THREADS: the number of concurrently allowed threads (defined by
	                        ThreadManager::kThreadLimit) has already been reached
	- \c B_BAD_THREAD_STATE: the thread has already been launched
	- other error code: failure
*/
status_t
ThreadManager::LaunchThread(RegistrarThread *thread)
{
	status_t err = thread ? B_OK : B_BAD_VALUE;
	if (!err)
		err = fThreadCount < kThreadLimit ? B_OK : B_NO_MORE_THREADS;

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
				}
			}
		}
	}
	if (!err)
		DBG(OUT("launched new %s thread, id == %ld\n", thread->Name(), thread->Id()));
	return err;
}

// CleanupThreads
/*! \brief Frees the resources of any threads that are no longer running
	
	\todo This function should perhaps be triggered periodically by a
	BMessageRunner once we have our own BMessageRunner implementation.
*/
status_t
ThreadManager::CleanupThreads()
{
	status_t err = B_ENTRY_NOT_FOUND;
	std::list<RegistrarThread*>::iterator i;
	for (i = fThreads.begin(); i != fThreads.end(); i++) {
		if (*i) {
			if ((*i)->IsFinished()) {
				OUT("Cleaning up thread %ld\n", (*i)->Id());
				RemoveThread(i);
				break;
			}			
		} else {
			OUT("WARNING: ThreadManager::CleanupThreads(): NULL mime_update_thread_shared_data "
				"pointer found in and removed from ThreadManager::fThreads list\n");
			fThreads.erase(i);
		}
	}			
	return err;
}

// ShutdownThreads
/*! \brief Requests that any running threads quit and frees the resources
	of any threads that are no longer running.

	\todo This function should be called during system shutdown around
	the same time as global B_QUIT_REQUESTED messages are sent out.
*/
status_t
ThreadManager::ShutdownThreads()
{
	status_t err = B_ENTRY_NOT_FOUND;
	std::list<RegistrarThread*>::iterator i;
	for (i = fThreads.begin(); i != fThreads.end(); i++) {
		if (*i) {
			if ((*i)->IsFinished()) 
				RemoveThread(i);
			else 
				(*i)->AskToExit();
		} else {
			OUT("WARNING: ThreadManager::ShutdownThreads(): NULL mime_update_thread_shared_data "
				"pointer found in and removed from ThreadManager::fThreads list\n");
			fThreads.erase(i);
		}
	}
	
	/*! \todo We may want to iterate back through the list at this point,
		snooze for a moment if find an unfinished thread, and kill it if
		it still isn't finished by the time we're done snoozing.
	*/
	
	return err;
}

// KillThreads
/*! \brief Kills any running threads and frees their resources.

	\todo This function should probably be called during system shutdown
	just before kernel shutdown begins.
*/
status_t
ThreadManager::KillThreads()
{
	std::list<RegistrarThread*>::iterator i;
	for (i = fThreads.begin(); i != fThreads.end(); i++) {
		if (*i) {
			if (!(*i)->IsFinished()) {
				status_t err = kill_thread((*i)->Id());
				if (err)
					OUT("WARNING: ThreadManager::KillThreads(): kill_thread(%ld) failed with "
						"error code 0x%lx\n", (*i)->Id(), err);
			}				
			RemoveThread(i);
		} else {
			OUT("WARNING: ThreadManager::KillThreads(): NULL mime_update_thread_shared_data "
				"pointer found in and removed from ThreadManager::fThreads list\n");
			fThreads.erase(i);
		}
	}
	return B_OK;
}

// RemoveThread
/*! \brief Deletes the given thread and removes it from the thread list.
*/
void
ThreadManager::RemoveThread(std::list<RegistrarThread*>::iterator &i)
{
	delete *i;
	fThreads.erase(i);
	fThreadCount--;
}

