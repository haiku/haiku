/*****************************************************************************/
// Jobs
//
// Author
//   Michael Pfeiffer
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2002 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef _JOBS_H
#define _JOBS_H

#include "BeUtils.h"
#include "ObjectList.h"
#include <Directory.h>
#include <Entry.h>
#include <Handler.h>
#include <String.h>
#include <StorageDefs.h>

enum JobStatus {  // job file
	kWaiting,     //   to be processed
	kProcessing,  //   processed by a printer add-on
	kFailed,      //   failed to process the job file
	kUnknown,     //   other
};

class Printer;

// Job file in printer folder
class Job : public Object {
private:
	BHandler* fHandler;   // the handler that watches the node of the job file
	BString fName;        // the name of the job file
	long fTime;           // the time encoded in the file name
	node_ref fNode;       // the node of the job file
	entry_ref fEntry;     // the entry of the job file
	JobStatus fStatus;    // the status of the job file
	bool fValid;          // is this a valid job file?
	Printer* fPrinter;    // the printer that processes this job
	
	void UpdateStatus(const char* status);
	void UpdateStatusAttribute(const char* status);
	bool HasAttribute(BNode* node, const char* name);
	bool IsValidJobFile();

public:
	Job(const BEntry& entry, BHandler* handler);

	status_t InitCheck() const        { return fTime >= 0 ? B_OK : B_ERROR; }
	
	// accessors
	const BString& Name() const       { return fName; }
	JobStatus Status() const          { return fStatus; }	
	bool IsValid() const              { return fValid; }
	long Time() const                 { return fTime; }
	const node_ref& NodeRef() const   { return fNode; }
	const entry_ref& EntryRef() const { return fEntry; }
	bool IsWaiting() const            { return fStatus == kWaiting; }
	Printer* GetPrinter() const       { return fPrinter; }

	// modifiers
	void SetPrinter(Printer* p) { fPrinter = p; }
	void SetStatus(JobStatus s, bool writeToNode = true);
	void UpdateAttribute();
	void Remove();

	void StartNodeWatching();
	void StopNodeWatching();
};


// Printer folder watches creation, deletion and attribute changes of job files
// and notifies print_server if a job is waiting for processing
class Folder : public BHandler {
	typedef BHandler inherited;
	
private:
	BDirectory fSpoolDir;
	BObjectList<Job> fJobs;
		
	static int AscendingByTime(const Job* a, const Job* b);

	bool AddJob(BEntry& entry);
	Job* Find(ino_t node);

	// node monitor handling
	void EntryCreated(BMessage* msg);
	void EntryRemoved(BMessage* msg);
	void AttributeChanged(BMessage* msg);
	void HandleNodeMonitorMessage(BMessage* msg);

	void NotifyPrintServer();

	void SetupJobList();
	
public:
	Folder(const BDirectory& spoolDir);
	~Folder();

	void MessageReceived(BMessage* msg);

	BDirectory* GetSpoolDir() { return &fSpoolDir; }

		// Caller is responsible to set the status of the job appropriately
		// and to release the object when done
	Job* GetNextJob();
};
#endif
