//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file ThreadManager.h
	ThreadManager interface declaration 
*/

#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <Handler.h>
#include <SupportDefs.h>

#include <list>

class RegistrarThread;

class ThreadManager : public BHandler {
public:
	ThreadManager();
	~ThreadManager();
	
	// BHandler virtuals
	virtual	void MessageReceived(BMessage* message);

	// Thread management functions
	status_t LaunchThread(RegistrarThread *thread);
	status_t CleanupThreads();
	status_t ShutdownThreads();
	status_t KillThreads();
	
private:
	static const uint kThreadLimit = 12;

	void RemoveThread(std::list<RegistrarThread*>::iterator &i);
	
	std::list<RegistrarThread*> fThreads;
	uint fThreadCount;
};

#endif	// THREAD_MANAGER_H
