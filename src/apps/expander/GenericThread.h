// license: public domain
// authors: jonas.sundstrom@kirilla.com
#ifndef _GENERIC_THREAD_H
#define _GENERIC_THREAD_H


#include <OS.h>
#include <Message.h>


class GenericThread {
public:
								GenericThread(
									const char* threadName = "generic_thread",
									int32 priority = B_NORMAL_PRIORITY,
									BMessage* message = NULL);
	virtual						~GenericThread(void);

			BMessage*			GetDataStore(void);
			void				SetDataStore(BMessage* message);

			status_t			Start(void);
			status_t			Pause(bool shouldBlock = true,
									bigtime_t timeout = 0);
			void				Quit(void);
			bool				IsPaused(void);
			bool				HasQuitBeenRequested(void);

			status_t			Suspend(void);
			status_t			Resume(void);
			status_t			Kill(void);

			void				ExitWithReturnValue(status_t returnValue);
			status_t			SetExitCallback(void (*callback)(void*),
									void* data);
			status_t			WaitForThread(status_t* exitValue);

			status_t			Rename(char* name);

			status_t			SendData(int32 code, void* buffer,
									size_t size);
			int32				ReceiveData(thread_id* sender, void* buffer,
									size_t size);
			bool				HasData(void);

			status_t			SetPriority(int32 priority);

			void				Snooze(bigtime_t delay);
			void				SnoozeUntil(bigtime_t delay,
									int timeBase = B_SYSTEM_TIMEBASE);

			status_t			GetInfo(thread_info* info);
			thread_id			GetThread(void);
			team_id				GetTeam(void);
			char*				GetName(void);
			thread_state		GetState(void);
			sem_id				GetSemaphore(void);
			int32				GetPriority(void);
			bigtime_t			GetUserTime(void);
			bigtime_t			GetKernelTime(void);
			void*				GetStackBase(void);
			void*				GetStackEnd(void);

protected:
	virtual	status_t			ThreadFunction(void);
	virtual	status_t			ThreadStartup(void);
	virtual	status_t			ExecuteUnit(void);
	virtual	status_t			ThreadShutdown(void);

	virtual	void				ThreadStartupFailed(status_t status);
	virtual	void				ExecuteUnitFailed(status_t status);
	virtual	void				ThreadShutdownFailed(status_t status);

			void				BeginUnit(void);
									// acquire m_execute_cycle
			void				EndUnit(void);
									// release m_execute_cycle

		BMessage*				fThreadDataStore;

private:
		static	 status_t		private_thread_function(void* pointer);

				thread_id		fThreadId;

				sem_id			fExecuteUnit;
									// acquire/relase within tread_function...
									// for Pause()

				bool			fQuitRequested;
				bool			fThreadIsPaused;
};


#endif	// _GENERIC_THREAD_H
