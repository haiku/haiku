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
// Copyright (c) 2002 Haiku Project
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
#include "FolderWatcher.h"

#include <Directory.h>
#include <Entry.h>
#include <Handler.h>
#include <Locker.h>
#include <String.h>
#include <StorageDefs.h>


enum JobStatus {  // job file
	kWaiting,     //   to be processed
	kProcessing,  //   processed by a printer add-on
	kFailed,      //   failed to process the job file
	kCompleted,   //   successfully processed
	kUnknown,     //   other
};

class Printer;
class Folder;

// Job file in printer folder
class Job : public Object {
private:
	Folder* fFolder;      // the handler that watches the node of the job file
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
	Job(const BEntry& entry, Folder* folder);

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
};


// Printer folder watches creation, deletion and attribute changes of job files
// and notifies print_server if a job is waiting for processing
class Folder : public FolderWatcher, public FolderListener {
	typedef FolderWatcher inherited;
	
private:
	BLocker* fLocker;
	BObjectList<Job> fJobs;
		
	static int AscendingByTime(const Job* a, const Job* b);

	bool AddJob(BEntry& entry, bool notify = true);
	Job* Find(node_ref* node);

	// FolderListener
	void EntryCreated(node_ref* node, entry_ref* entry);
	void EntryRemoved(node_ref* node);
	void AttributeChanged(node_ref* node);

	void SetupJobList();

protected:
	enum {
		kJobAdded,
		kJobRemoved,
		kJobAttrChanged,
	};
	virtual void Notify(Job* job, int kind) = 0;
	
public:
	Folder(BLocker* fLocker, BLooper* looper, const BDirectory& spoolDir);
	~Folder();

	BDirectory* GetSpoolDir() { return inherited::Folder(); }
	
	BLocker* Locker() const { return fLocker; }
	bool Lock() const       { return fLocker != NULL ? fLocker->Lock() : true; }
	void Unlock() const     { if (fLocker) fLocker->Unlock(); }

	int32 CountJobs() const { return fJobs.CountItems(); }
	Job* JobAt(int i) const { return fJobs.ItemAt(i); }

		// Caller is responsible to set the status of the job appropriately
		// and to release the object when done
	Job* GetNextJob();
};
#endif
