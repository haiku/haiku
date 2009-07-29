#ifndef __GENERIC_THREAD_H__
#define __GENERIC_THREAD_H__


#include <Message.h>


class GenericThread
{
public:
							GenericThread(const char* 
								thread_name = "generic_thread",
								int32 priority = B_NORMAL_PRIORITY,
								BMessage* message = NULL);
	virtual					~GenericThread();

			BMessage*		GetDataStore();
			void			SetDataStore(BMessage* message);

			status_t		Start();
			status_t		Pause(bool doBlock = TRUE, bigtime_t timeout = 0);
			void			Quit();
			bool			IsPaused();
			bool			HasQuitBeenRequested();
			
			status_t		Suspend();
			status_t		Resume();
			status_t		Kill();
			
			void			ExitWithReturnValue(status_t returnValue);
			status_t		SetExitCallback(void (*callback)(void*),
								void* data);
			status_t		WaitForThread(status_t* exitValue);
			
			status_t		Rename(char* name);
			
			status_t		SendData(int32 code, void* buffer,
								size_t bufferSize);
			int32			ReceiveData(thread_id* sender, void* buffer,
								size_t bufferSize);
			bool			HasData();
			
			status_t		SetPriority(int32 newPriority);

			void			Snooze(bigtime_t microseconds);
			void			SnoozeUntil(bigtime_t microseconds,
								int timebase = B_SYSTEM_TIMEBASE);
				
			status_t		GetInfo(thread_info* threadInfo);
			thread_id		GetThread();
			team_id			GetTeam();
			char*			GetName();
			thread_state	GetState();
			sem_id			GetSemaphore();
			int32			GetPriority();
			bigtime_t		GetUserTime();
			bigtime_t		GetKernelTime();
			void*			GetStackBase();
			void*			GetStackEnd();

protected:
	virtual	status_t		ThreadFunction();
	virtual	status_t		ThreadStartup();
	virtual	status_t		ExecuteUnit();
	virtual	status_t		ThreadShutdown();
	
	virtual	void			ThreadStartupFailed(status_t status);
	virtual	void			ExecuteUnitFailed(status_t status);
	virtual	void			ThreadShutdownFailed(status_t status);

			void			BeginUnit();
			void			EndUnit();

			BMessage*		fThreadDataStore;
				
private:
	static	status_t		_ThreadFunction(void* simpleThreadPtr);
		
			thread_id		fThreadId;
			sem_id			fExecuteUnitSem;
			bool			fQuitRequested;
			bool			fThreadIsPaused;

};

#endif	// __GENERIC_THREAD_H__

