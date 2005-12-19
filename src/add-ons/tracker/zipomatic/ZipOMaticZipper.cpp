// license: public domain
// authors: jonas.sundstrom@kirilla.com
// 
// exception: ZipperThread::PipeCommand()
// written by Peter Folk, published in the BeDevTalk mailinglist FAQ
// License unknown, most likely public domain

#include <Debug.h>

#include <Path.h>

#include "ZipOMaticZipper.h"
#include "ZipOMaticWindow.h"
#include "ZipOMaticMisc.h"

#include <signal.h>
#include <unistd.h>
#include <errno.h>

const char * ZipperThreadName	=	"ZipperThread";


ZipperThread::ZipperThread (BMessage * a_refs_message, BWindow * a_window)
:	GenericThread(ZipperThreadName, B_NORMAL_PRIORITY, a_refs_message),
	m_window					(a_window),
	m_window_messenger			(new BMessenger(NULL, a_window)),
	m_zip_process_thread_id		(-1),
	m_std_in					(-1),
	m_std_out					(-1),
	m_std_err					(-1),
	m_zip_output				(NULL),
	m_zip_output_string			(),
	m_zip_output_buffer			(new char [4096])
{
	PRINT(("ZipperThread()\n"));
	
	m_thread_data_store = 	new BMessage (* a_refs_message); // leak?
															// prevents bug with B_SIMPLE_DATA
															// (drag&drop messages)
}

ZipperThread::~ZipperThread()	
{
	delete m_window_messenger;
	delete [] m_zip_output_buffer;
}

status_t
ZipperThread::ThreadStartup (void)
{
	PRINT(("ZipperThread::ThreadStartup()\n"));

	//	init
	status_t	status	=	B_OK;
	entry_ref	ref;
	entry_ref	last_ref;
	bool		same_folder	=	true;

	BString 	zip_archive_filename	=	"Archive.zip";
	
	// do all refs have the same parent dir?
	type_code	ref_type	=	B_REF_TYPE;
	int32		ref_count	=	0;
	
	status  =  m_thread_data_store->GetInfo("refs", & ref_type, & ref_count);
	if (status != B_OK)
		return status;
	
	for (int index = 0;	index < ref_count ;	index++)
	{
		m_thread_data_store->FindRef("refs", index, & ref);
		
		if (index > 0)
		{
			BEntry	entry (& ref);
			if (entry.IsSymLink())
			{
				entry.SetTo(& ref, true);
				entry_ref  target;
				entry.GetRef(& target);
				if (last_ref.directory != target.directory)
				{
					same_folder  =  false;
					break;
				}
			}
			else
			if (last_ref.directory != ref.directory)
			{
				same_folder	=	false;
				break;
			}
		}
		last_ref =	ref;
	}

	// change active dir
	if (same_folder)
	{
		BEntry entry (& last_ref);
		BPath path;
		entry.GetParent(& entry);
		entry.GetPath(& path);
		chdir(path.Path());
	}
	else
	{
		BPath path;
		if (find_directory(B_DESKTOP_DIRECTORY, & path) == B_OK)
			chdir (path.Path());
	}

	// archive filename
	if (ref_count == 1)
	{
		zip_archive_filename	=	last_ref.name;
		zip_archive_filename	+=	".zip";			
	}
	
	int32  argc  =  ref_count + 3;
	const char ** argv =	 new const char * [argc + 1];
	
	argv[0]	=	strdup("/bin/zip");
	argv[1]	=	strdup("-ry");
	argv[2]	=	strdup(zip_archive_filename.String());
	
	
	// files to zip
	for (int ref_index = 0;  ref_index < ref_count ;  ref_index++)
	{
		m_thread_data_store->FindRef("refs", ref_index, & ref);
	
		if (same_folder)				
		{
			argv[3 + ref_index]	=	strdup(ref.name);	// just the file name
		}
		else							
		{
			BPath	path	(& ref);
			BString file	=	path.Path();
			argv[3 + ref_index]	=	strdup(path.Path());	// full path
		}
	}
	
	argv[argc]	=	NULL;

	m_zip_process_thread_id  =  PipeCommand (argc, argv, m_std_in, m_std_out, m_std_err); 

	delete [] argv;

	if (m_zip_process_thread_id < 0)
		return (m_zip_process_thread_id); 
    	
    resume_thread (m_zip_process_thread_id); 
    
	m_zip_output  =  fdopen (m_std_out, "r");	

	zip_archive_filename.Prepend("Creating archive: ");
	
	SendMessageToWindow ('strt', "archive_filename", zip_archive_filename.String());
	SendMessageToWindow ('outp', "zip_output", "Preparing to archive"); 

	PRINT(("\n"));

	return B_OK;
}

status_t
ZipperThread::ExecuteUnit (void)
{
	//PRINT(("ZipperThread::ExecuteUnit()\n"));

	// read output from /bin/zip
	// send it to window
	
	char * output_string;	
	output_string =	fgets(m_zip_output_buffer , 4096-1, m_zip_output);

	if (output_string == NULL)
	{
		return EOF;
	}
	else
	{
		char * new_line_ptr = strrchr(output_string, '\n');
		if (new_line_ptr != NULL)
			*new_line_ptr = '\0';
	
		if (! strncmp("  a", m_zip_output_buffer, 3))
		{
			m_zip_output_buffer[2] = 'A';
			SendMessageToWindow ('outp', "zip_output", output_string + 2);
		}
		else if (! strncmp("up", m_zip_output_buffer, 2))
		{
			m_zip_output_buffer[0] = 'U';
			SendMessageToWindow ('outp', "zip_output", output_string);
		}
		else
		{
			SendMessageToWindow ('outp', "zip_output", output_string);
		}
	}

	return B_OK;
}

status_t
ZipperThread::ThreadShutdown (void)
{
	PRINT(("ZipperThread::ThreadShutdown()\n"));

	close(m_std_in); 
    close(m_std_out); 
   	close(m_std_err);

	return B_OK;
}

void
ZipperThread::ThreadStartupFailed (status_t a_status)
{
	error_message("ZipperThread::ThreadStartupFailed() \n", a_status);

	Quit();
}

void
ZipperThread::ExecuteUnitFailed (status_t a_status)
{
	error_message("ZipperThread::ExecuteUnitFailed() \n", a_status);
	
	
	if (a_status == EOF)
	{
		// thread has finished, been quit or killed, we don't know
		SendMessageToWindow ('exit');
	}
	else
	{
		// explicit error - communicate error to Window
		SendMessageToWindow ('exrr');
	}
	
	Quit();
}

void
ZipperThread::ThreadShutdownFailed (status_t a_status)
{
	error_message("ZipperThread::ThreadShutdownFailed() \n", a_status);
}


void
ZipperThread::MakeShellSafe	(BString * a_string)
{
	a_string->CharacterEscape("\"$`", '\\');
	a_string->Prepend("\""); 
	a_string->Append("\""); 
}

status_t
ZipperThread::ProcessRefs	(BMessage * msg)
{

	return B_OK;
}

thread_id
ZipperThread::PipeCommand (int argc, const char **argv, int & in, int & out, int & err, const char **envp) 
{ 
	// This function written by Peter Folk <pfolk@uni.uiuc.edu>
	// and published in the BeDevTalk FAQ 
	// http://www.abisoft.com/faq/BeDevTalk_FAQ.html#FAQ-209

    // Save current FDs 
    int old_in  =  dup(0); 
    int old_out  =  dup(1); 
    int old_err  =  dup(2); 
    
    int filedes[2]; 
    
    /* Create new pipe FDs as stdin, stdout, stderr */ 
    pipe(filedes);  dup2(filedes[0],0); close(filedes[0]); 
    in=filedes[1];  // Write to in, appears on cmd's stdin 
    pipe(filedes);  dup2(filedes[1],1); close(filedes[1]); 
    out=filedes[0]; // Read from out, taken from cmd's stdout 
    pipe(filedes);  dup2(filedes[1],2); close(filedes[1]); 
    err=filedes[0]; // Read from err, taken from cmd's stderr 
        
    // "load" command. 
    thread_id ret  =  load_image(argc, argv, envp); 
    // thread ret is now suspended. 

	PRINT(("load_image() thread_id: %ld\n", ret));

    // Restore old FDs 
    close(0); dup(old_in); close(old_in); 
    close(1); dup(old_out); close(old_out); 
    close(2); dup(old_err); close(old_err); 
    
    /* Theoretically I should do loads of error checking, but 
       the calls aren't very likely to fail, and that would 
       muddy up the example quite a bit.  YMMV. */ 

    return ret; 
} 

void
ZipperThread::SendMessageToWindow (uint32 a_msg_what, const char * a_string_name, const char * a_string_value) 
{ 
	BMessage msg (a_msg_what);
	
	if (! (a_string_name == NULL || a_string_value == NULL))
		msg.AddString(a_string_name, a_string_value);
	
	m_window_messenger->SendMessage(& msg);
}

status_t
ZipperThread::SuspendExternalZip (void) 
{ 
	PRINT(("ZipperThread::SuspendExternalZip()\n"));

	status_t  status  =  B_OK;
	thread_info  zip_thread_info;
	status  =  get_thread_info (m_zip_process_thread_id, & zip_thread_info);
	BString	thread_name  =  zip_thread_info.name;
	
	if (status == B_OK && thread_name == "zip")
		return suspend_thread (m_zip_process_thread_id);
	else
		return status;
}

status_t
ZipperThread::ResumeExternalZip (void) 
{ 
	PRINT(("ZipperThread::ResumeExternalZip()\n"));

	status_t  status  =  B_OK;
	thread_info  zip_thread_info;
	status  =  get_thread_info (m_zip_process_thread_id, & zip_thread_info);
	BString	thread_name  =  zip_thread_info.name;
	
	if (status == B_OK && thread_name == "zip")
		return resume_thread (m_zip_process_thread_id);
	else
		return status;
}

status_t
ZipperThread::InterruptExternalZip (void) 
{ 
	PRINT(("ZipperThread::InterruptExternalZip()\n"));
	
	status_t  status  =  B_OK;
	thread_info  zip_thread_info;
	status  =  get_thread_info (m_zip_process_thread_id, & zip_thread_info);
	BString	thread_name  =  zip_thread_info.name;
	
	if (status == B_OK && thread_name == "zip")
	{
		status  =  B_OK;
		status  =  send_signal (m_zip_process_thread_id, SIGINT);
		WaitOnExternalZip();
		return status;
	}
	else
		return status;
}

status_t
ZipperThread::WaitOnExternalZip (void) 
{ 
	PRINT(("ZipperThread::WaitOnExternalZip()\n"));
	
	status_t  status  =  B_OK;
	thread_info  zip_thread_info;
	status  =  get_thread_info (m_zip_process_thread_id, & zip_thread_info);
	BString	thread_name  =  zip_thread_info.name;
	
	if (status == B_OK && thread_name == "zip")
		return wait_for_thread (m_zip_process_thread_id, & status);
	else
		return status;
}
