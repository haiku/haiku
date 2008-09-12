#ifndef __GENERIC_THREAD_H__
#define __GENERIC_THREAD_H__

#include <OS.h>
#include <Message.h>

class GenericThread
{
	public:
		GenericThread(const char * thread_name = "generic_thread", int32 a_priority = B_NORMAL_PRIORITY, BMessage * message = NULL);
		virtual	~GenericThread(void);

		BMessage *	GetDataStore(void);
		void		SetDataStore(BMessage * message);

		status_t	Start(void);
		status_t	Pause(bool a_do_block = TRUE, bigtime_t a_timeout = 0);
		void		Quit(void);
		bool		IsPaused(void);
		bool		HasQuitBeenRequested(void);

		status_t	Suspend(void);
		status_t	Resume(void);
		status_t	Kill(void);

		void		ExitWithReturnValue(status_t a_return_value);
		status_t	SetExitCallback(void (* a_callback)(void *), void * a_data);
		status_t	WaitForThread(status_t * a_exit_value);

		status_t	Rename(char * a_name);

		status_t	SendData(int32 a_code, void * a_buffer, size_t a_buffer_size);
		int32		ReceiveData(thread_id * a_sender, void * a_buffer, size_t a_buffer_size);
		bool		HasData(void);

		status_t	SetPriority(int32 a_new_priority);

		void		Snooze(bigtime_t a_microseconds);
		void		SnoozeUntil(bigtime_t a_microseconds, int a_timebase = B_SYSTEM_TIMEBASE);


		status_t		GetInfo(thread_info * thread_info);
		thread_id		GetThread(void);
		team_id			GetTeam(void);
		char	*		GetName(void);
		thread_state	GetState(void);
		sem_id			GetSemaphore(void);
		int32			GetPriority(void);
		bigtime_t		GetUserTime(void);
		bigtime_t		GetKernelTime(void);
		void	*		GetStackBase(void);
		void	*		GetStackEnd(void);

	protected:

		virtual status_t	ThreadFunction(void);
		virtual status_t    ThreadStartup(void);
		virtual status_t	ExecuteUnit(void);
		virtual status_t	ThreadShutdown(void);

		virtual void		ThreadStartupFailed(status_t a_status);
		virtual void		ExecuteUnitFailed(status_t a_status);
		virtual void		ThreadShutdownFailed(status_t a_status);

		void		BeginUnit(void);	// acquire m_execute_cycle
		void		EndUnit(void);	// release m_execute_cycle

		BMessage	 *		fThreadDataStore;

	private:

		static status_t	private_thread_function(void * a_simple_thread_ptr);

		thread_id		fThreadId;

		sem_id			fExecuteUnit;	// acq./relase within tread_function.. For Pause()

		bool			fQuitRequested;
		bool			fThreadIsPaused;

};

#endif
