//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file RegistrarThreadManager.h
	RegistrarThreadManager interface declaration
*/

#ifndef REGISTRAR_THREAD_MANAGER_H
#define REGISTRAR_THREAD_MANAGER_H

#include <Handler.h>
#include <SupportDefs.h>

#include <list>

class RegistrarThread;

class RegistrarThreadManager : public BHandler {
public:
	RegistrarThreadManager();
	~RegistrarThreadManager();

	// BHandler virtuals
	virtual	void MessageReceived(BMessage* message);

	// Thread management functions
	status_t LaunchThread(RegistrarThread *thread);
	status_t CleanupThreads();
	status_t ShutdownThreads();
	status_t KillThreads();

	uint ThreadCount() const;

	static const int kThreadLimit = 12;
private:

	std::list<RegistrarThread*>::iterator&
		RemoveThread(std::list<RegistrarThread*>::iterator &i);

	std::list<RegistrarThread*> fThreads;
	int32 fThreadCount;
};

#endif	// THREAD_MANAGER_H
