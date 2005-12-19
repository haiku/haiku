#ifndef __ZIPOMATIC_ZIPPER_H__
#define __ZIPOMATIC_ZIPPER_H__

#include <Message.h>
#include <Volume.h>
#include <String.h>
#include <Window.h>
#include <OS.h> 
#include <FindDirectory.h> 

#include "GenericThread.h"
#include <stdlib.h>

extern const char * ZipperThreadName;

class ZipperThread : public GenericThread
{
public:
					ZipperThread		(BMessage * a_refs_message, BWindow * a_window);
					~ZipperThread		();

		status_t			SuspendExternalZip		(void);
		status_t			ResumeExternalZip		(void);
		status_t			InterruptExternalZip	(void);
		status_t			WaitOnExternalZip		(void);

private:

		virtual status_t    ThreadStartup			(void);
		virtual status_t	ExecuteUnit				(void);
		virtual status_t	ThreadShutdown			(void);
		
		virtual void		ThreadStartupFailed		(status_t a_status);
		virtual void		ExecuteUnitFailed		(status_t a_status);
		virtual void		ThreadShutdownFailed	(status_t a_status);

		status_t			ProcessRefs				(BMessage *	msg);
		void				MakeShellSafe			(BString * a_string);
		
		thread_id			PipeCommand				(int argc, const char **argv, 
													 int & in, int & out, int & err,
													 const char **envp = (const char **) environ);
	
		void				SendMessageToWindow		(uint32 a_msg_what,
													 const char * a_string_name  =  NULL,
													 const char * a_string_value  =  NULL);
		
		BWindow	*		m_window;
		BMessenger * 	m_window_messenger;

		int32			m_zip_process_thread_id;
		int				m_std_in;
		int				m_std_out;
		int				m_std_err;
		FILE *			m_zip_output;
		BString			m_zip_output_string;
		char *			m_zip_output_buffer;
};

#endif // __ZIPOMATIC_ZIPPER_H__

