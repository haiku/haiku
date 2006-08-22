/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas.sundstrom@kirilla.com
 */


#include "ZipOMaticZipper.h"
#include "ZipOMaticWindow.h"
#include "ZipOMaticMisc.h"

#include <Debug.h>
#include <FindDirectory.h> 
#include <Message.h>
#include <Path.h>
#include <Volume.h>

#include <String.h>
#include <Window.h>
#include <OS.h>

#include <signal.h>
#include <unistd.h>
#include <errno.h>


const char* kZipperThreadName = "ZipperThread";


ZipperThread::ZipperThread (BMessage* refsMessage, BWindow* window)
	: GenericThread(kZipperThreadName, B_NORMAL_PRIORITY, refsMessage),
	fWindowMessenger(window),
	m_zip_process_thread_id(-1),
	m_std_in(-1),
	m_std_out(-1),
	m_std_err(-1),
	fOutputFile(NULL)
{
	PRINT(("ZipperThread()\n"));

	m_thread_data_store = new BMessage(*refsMessage);
		// leak?
		// prevents bug with B_SIMPLE_DATA
		// (drag&drop messages)
}


ZipperThread::~ZipperThread()	
{
}


status_t
ZipperThread::ThreadStartup()
{
	PRINT(("ZipperThread::ThreadStartup()\n"));

	// init
	status_t status	= B_OK;
	entry_ref ref;
	entry_ref last_ref;
	bool same_folder = true;

	BString zip_archive_filename = "Archive.zip";

	// do all refs have the same parent dir?
	type_code ref_type = B_REF_TYPE;
	int32 ref_count = 0;

	status = m_thread_data_store->GetInfo("refs", &ref_type, &ref_count);
	if (status != B_OK)
		return status;

	for (int index = 0;	index < ref_count ;	index++) {
		m_thread_data_store->FindRef("refs", index, & ref);
		
		if (index > 0) {
			BEntry entry(&ref);
			if (entry.IsSymLink()) {
				entry.SetTo(& ref, true);
				entry_ref  target;
				entry.GetRef(& target);
				if (last_ref.directory != target.directory) {
					same_folder = false;
					break;
				}
			} else if (last_ref.directory != ref.directory) {
				same_folder = false;
				break;
			}
		}
		last_ref = ref;
	}

	// change active dir
	if (same_folder) {
		BEntry entry(&last_ref);
		BPath path;
		entry.GetParent(&entry);
		entry.GetPath(&path);
		chdir(path.Path());
	} else {
		BPath path;
		if (find_directory(B_DESKTOP_DIRECTORY, &path) == B_OK)
			chdir(path.Path());
	}

	// archive filename
	if (ref_count == 1) {
		zip_archive_filename = last_ref.name;
		zip_archive_filename += ".zip";			
	}

	int32 argc = ref_count + 3;
	const char** argv = new const char* [argc + 1];

	argv[0] = strdup("/bin/zip");
	argv[1] = strdup("-ry");
	argv[2] = strdup(zip_archive_filename.String());

	// files to zip
	for (int ref_index = 0;  ref_index < ref_count ;  ref_index++) {
		m_thread_data_store->FindRef("refs", ref_index, &ref);

		if (same_folder) {
			argv[3 + ref_index]	= strdup(ref.name);	// just the file name
		} else {
			BPath path(&ref);
			BString file = path.Path();
			argv[3 + ref_index]	= strdup(path.Path());	// full path
		}
	}

	argv[argc] = NULL;

	m_zip_process_thread_id = PipeCommand(argc, argv, m_std_in, m_std_out, m_std_err); 

	delete [] argv;

	if (m_zip_process_thread_id < 0)
		return m_zip_process_thread_id;

	resume_thread(m_zip_process_thread_id);

	fOutputFile = fdopen(m_std_out, "r");	

	zip_archive_filename.Prepend("Creating archive: ");

	SendMessageToWindow ('strt', "archive_filename", zip_archive_filename.String());
	SendMessageToWindow ('outp', "zip_output", "Preparing to archive"); 

	PRINT(("\n"));

	return B_OK;
}


status_t
ZipperThread::ExecuteUnit()
{
	//PRINT(("ZipperThread::ExecuteUnit()\n"));

	// read output from /bin/zip
	// send it to window
	char buffer[4096];

	char* output = fgets(buffer, sizeof(buffer) - 1, fOutputFile);
	if (output == NULL)
		return EOF;

	char* newLine = strrchr(output, '\n');
	if (newLine != NULL)
		*newLine = '\0';

	if (!strncmp("  a", output, 3)) {
		output[2] = 'A';
		SendMessageToWindow('outp', "zip_output", output + 2);
	} else if (!strncmp("up", output, 2)) {
		output[0] = 'U';
		SendMessageToWindow('outp', "zip_output", output);
	} else {
		SendMessageToWindow('outp', "zip_output", output);
	}

	return B_OK;
}


status_t
ZipperThread::ThreadShutdown()
{
	PRINT(("ZipperThread::ThreadShutdown()\n"));

	close(m_std_in); 
    close(m_std_out); 
   	close(m_std_err);

	return B_OK;
}


void
ZipperThread::ThreadStartupFailed(status_t status)
{
	error_message("ZipperThread::ThreadStartupFailed() \n", status);
	Quit();
}


void
ZipperThread::ExecuteUnitFailed(status_t status)
{
	error_message("ZipperThread::ExecuteUnitFailed() \n", status);

	if (status == EOF) {
		// thread has finished, been quit or killed, we don't know
		SendMessageToWindow('exit');
	} else {
		// explicit error - communicate error to Window
		SendMessageToWindow('exrr');
	}

	Quit();
}


void
ZipperThread::ThreadShutdownFailed(status_t status)
{
	error_message("ZipperThread::ThreadShutdownFailed() \n", status);
}


void
ZipperThread::MakeShellSafe(BString* string)
{
	string->CharacterEscape("\"$`", '\\');
	string->Prepend("\""); 
	string->Append("\""); 
}


status_t
ZipperThread::ProcessRefs(BMessage* msg)
{
	return B_OK;
}


thread_id
ZipperThread::PipeCommand(int argc, const char** argv, int& in, int& out,
	int& err, const char** envp)
{
	// This function written by Peter Folk <pfolk@uni.uiuc.edu>
	// and published in the BeDevTalk FAQ 
	// http://www.abisoft.com/faq/BeDevTalk_FAQ.html#FAQ-209

	// Save current FDs 
	int old_in = dup(0); 
	int old_out = dup(1); 
	int old_err = dup(2); 

	int filedes[2]; 

	/* Create new pipe FDs as stdin, stdout, stderr */ 
	pipe(filedes);
	dup2(filedes[0], 0);
	close(filedes[0]); 
	in = filedes[1];  // Write to in, appears on cmd's stdin 
	pipe(filedes);
	dup2(filedes[1], 1);
	close(filedes[1]); 
	out = filedes[0]; // Read from out, taken from cmd's stdout 
	pipe(filedes);
	dup2(filedes[1], 2);
	close(filedes[1]); 
	err = filedes[0]; // Read from err, taken from cmd's stderr 

	// "load" command. 
	thread_id ret = load_image(argc, argv, envp); 
	// thread ret is now suspended. 

	PRINT(("load_image() thread_id: %ld\n", ret));

	// Restore old FDs 
	close(0); dup(old_in); close(old_in); 
	close(1); dup(old_out); close(old_out); 
	close(2); dup(old_err); close(old_err); 

	// TODO:
	/*
	Theoretically I should do loads of error checking, but 
	the calls aren't very likely to fail, and that would 
	muddy up the example quite a bit.  YMMV.
	*/ 

    return ret;
}


void
ZipperThread::SendMessageToWindow(uint32 what, const char* name, const char* value)
{
	BMessage msg(what);
	if (name != NULL && value != NULL)
		msg.AddString(name, value);

	fWindowMessenger.SendMessage(&msg);
}


status_t
ZipperThread::SuspendExternalZip()
{
	PRINT(("ZipperThread::SuspendExternalZip()\n"));

	status_t status = B_OK;
	thread_info zip_thread_info;
	status = get_thread_info(m_zip_process_thread_id, &zip_thread_info);
	BString	thread_name = zip_thread_info.name;

	if (status == B_OK && thread_name == "zip")
		return suspend_thread (m_zip_process_thread_id);

	return status;
}


status_t
ZipperThread::ResumeExternalZip()
{
	PRINT(("ZipperThread::ResumeExternalZip()\n"));

	status_t status = B_OK;
	thread_info zip_thread_info;
	status = get_thread_info(m_zip_process_thread_id, &zip_thread_info);
	BString	thread_name = zip_thread_info.name;

	if (status == B_OK && thread_name == "zip")
		return resume_thread(m_zip_process_thread_id);

	return status;
}


status_t
ZipperThread::InterruptExternalZip()
{
	PRINT(("ZipperThread::InterruptExternalZip()\n"));
	
	status_t status = B_OK;
	thread_info zip_thread_info;
	status = get_thread_info(m_zip_process_thread_id, &zip_thread_info);
	BString	thread_name = zip_thread_info.name;

	if (status == B_OK && thread_name == "zip") {
		status = B_OK;
		status = send_signal(m_zip_process_thread_id, SIGINT);
		WaitOnExternalZip();
		return status;
	}

	return status;
}


status_t
ZipperThread::WaitOnExternalZip()
{
	PRINT(("ZipperThread::WaitOnExternalZip()\n"));

	status_t status = B_OK;
	thread_info zip_thread_info;
	status = get_thread_info(m_zip_process_thread_id, &zip_thread_info);
	BString	thread_name = zip_thread_info.name;

	if (status == B_OK && thread_name == "zip")
		return wait_for_thread(m_zip_process_thread_id, &status);

	return status;
}
