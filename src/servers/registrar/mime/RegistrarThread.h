//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file RegistrarThread.h
	RegistrarThread interface declaration
*/

#ifndef REGISTRAR_THREAD_H
#define REGISTRAR_THREAD_H

#include <Messenger.h>
#include <OS.h>

class RegistrarThread {
public:
	RegistrarThread(const char *name, int32 priority,
		BMessenger managerMessenger);
	virtual ~RegistrarThread();

	virtual status_t InitCheck();
	status_t Run();

	thread_id Id() const;
	const char* Name() const;

	void AskToExit();
	bool IsFinished() const;

protected:
	//! The function executed in the object's thread when Run() is called
	virtual status_t ThreadFunction() = 0;

	BMessenger fManagerMessenger;
	bool fShouldExit;	// Initially false, may be set to true by AskToExit()
	bool fIsFinished;	// Initially false, set to true by the thread itself
						// upon completion
private:
	static int32 EntryFunction(void *data);

	status_t fStatus;
	thread_id fId;
	char fName[B_OS_NAME_LENGTH];
	int32 fPriority;
};

#endif	// REGISTRAR_THREAD_H
