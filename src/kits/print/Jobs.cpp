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


#include "pr_server.h"
#include "Jobs.h"
// #include "PrintServerApp.h"

// posix
#include <stdlib.h>
#include <string.h>

// BeOS
#include <kernel/fs_attr.h>
#include <Application.h>
#include <Node.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>


// Implementation of Job

Job::Job(const BEntry& job, Folder* folder) 
	: fFolder(folder)
	, fTime(-1)
	, fStatus(kUnknown)
	, fValid(false)
	, fPrinter(NULL)
{
	// store light weight versions of BEntry and BNode
	job.GetRef(&fEntry);
	job.GetNodeRef(&fNode);

	fValid = IsValidJobFile();
	
	BNode node(&job);
	if (node.InitCheck() != B_OK) return;

	BString status;
		// read status attribute
	if (node.ReadAttrString(PSRV_SPOOL_ATTR_STATUS, &status) != B_OK) {
		status = "";
	}
    UpdateStatus(status.String());
    
    	// Now get file name and creation time from file name
    fTime = 0;
    BEntry entry(job);
    char name[B_FILE_NAME_LENGTH];
    if (entry.InitCheck() == B_OK && entry.GetName(name) == B_OK) {
    	fName = name;
    		// search for last '@' in file name
		char* p = NULL, *c = name;
		while ((c = strchr(c, '@')) != NULL) {
			p = c; c ++;
		}
			// and get time from file name
		if (p) fTime = atoi(p+1);
    }
}

// conversion from string representation of status to JobStatus constant
void Job::UpdateStatus(const char* status) {
	if (strcmp(status, PSRV_JOB_STATUS_WAITING) == 0) fStatus = kWaiting;
	else if (strcmp(status, PSRV_JOB_STATUS_PROCESSING) == 0) fStatus = kProcessing;
	else if (strcmp(status, PSRV_JOB_STATUS_FAILED) == 0) fStatus = kFailed;
	else if (strcmp(status, PSRV_JOB_STATUS_COMPLETED) == 0) fStatus = kCompleted;
	else fStatus = kUnknown;
}

// Write to status attribute of node
void Job::UpdateStatusAttribute(const char* status) {
	BNode node(&fEntry);
	if (node.InitCheck() == B_OK) {
		node.WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, status, strlen(status)+1);
	}
}


bool Job::HasAttribute(BNode* n, const char* name) {
	attr_info info;
	return n->GetAttrInfo(name, &info) == B_OK;
}


bool Job::IsValidJobFile() {
	BNode node(&fEntry);
	if (node.InitCheck() != B_OK) return false;

	BNodeInfo info(&node);
	char mimeType[256];

		// Is job a spool file?
	return (info.InitCheck() == B_OK && 
	    info.GetType(mimeType) == B_OK &&
	    strcmp(mimeType, PSRV_SPOOL_FILETYPE) == 0) &&
	    HasAttribute(&node, PSRV_SPOOL_ATTR_MIMETYPE) &&
	    HasAttribute(&node, PSRV_SPOOL_ATTR_PAGECOUNT) &&
	    HasAttribute(&node, PSRV_SPOOL_ATTR_DESCRIPTION) &&
	    HasAttribute(&node, PSRV_SPOOL_ATTR_PRINTER) &&
	    HasAttribute(&node, PSRV_SPOOL_ATTR_STATUS); 
}


// Set status of object and optionally write to attribute of node
void Job::SetStatus(JobStatus s, bool writeToNode) {
	fStatus = s; 
	if (!writeToNode) return;
	switch (s) {
		case kWaiting: UpdateStatusAttribute(PSRV_JOB_STATUS_WAITING); break;
		case kProcessing: UpdateStatusAttribute(PSRV_JOB_STATUS_PROCESSING); break;
		case kFailed: UpdateStatusAttribute(PSRV_JOB_STATUS_FAILED); break;
		case kCompleted: UpdateStatusAttribute(PSRV_JOB_STATUS_COMPLETED); break;
		default: break;
	}
}

// Synchronize file attribute with member variable
void Job::UpdateAttribute() {
	fValid = fValid || IsValidJobFile();	
	BNode node(&fEntry);
	BString status;
	if (node.InitCheck() == B_OK &&
		node.ReadAttrString(PSRV_SPOOL_ATTR_STATUS, &status) == B_OK) {
		UpdateStatus(status.String());
	}
}

void Job::Remove() {
	BEntry entry(&fEntry);
	if (entry.InitCheck() == B_OK) entry.Remove();
}

// Implementation of Folder

// BObjectList CompareFunction
int Folder::AscendingByTime(const Job* a, const Job* b) {
	return a->Time() - b->Time();
}

bool Folder::AddJob(BEntry& entry, bool notify) {
	Job* job = new Job(entry, this);
	if (job->InitCheck() == B_OK) {
		fJobs.AddItem(job);
		if (notify) Notify(job, kJobAdded);
		return true;
	} else {
		job->Release();
		return false;
	}
}

// simplified assumption that ino_t identifies job file
// will probabely not work in all cases with link to a file on another volume???
Job* Folder::Find(node_ref* node) {
	for (int i = 0; i < fJobs.CountItems(); i ++) {
		Job* job = fJobs.ItemAt(i);
		if (job->NodeRef() == *node) return job;
	}
	return NULL;
}

void Folder::EntryCreated(node_ref* node, entry_ref* entry) {
	BEntry job(entry);
	if (job.InitCheck() == B_OK && Lock()) {
		if (AddJob(job)) {
			fJobs.SortItems(AscendingByTime);
		}
		Unlock();
	}
}

void Folder::EntryRemoved(node_ref* node) {
	Job* job = Find(node);
	if (job && Lock()) {
		fJobs.RemoveItem(job);
		Notify(job, kJobRemoved);
		job->Release();
		Unlock();
	}
}

void Folder::AttributeChanged(node_ref* node) {
	Job* job = Find(node);
	if (job && Lock()) {
		job->UpdateAttribute();
		Notify(job, kJobAttrChanged);
		Unlock();
	}
}

// initial setup of job list
void Folder::SetupJobList() {
	if (inherited::Folder()->InitCheck() == B_OK) {
		inherited::Folder()->Rewind();
		
		BEntry entry;
		while (inherited::Folder()->GetNextEntry(&entry) == B_OK) {
			AddJob(entry, false);
		}
		fJobs.SortItems(AscendingByTime);
	}	
}

Folder::Folder(BLocker* locker, BLooper* looper, const BDirectory& spoolDir) 
	: FolderWatcher(looper, spoolDir, true)
	, fLocker(locker)
	, fJobs()
{
	SetListener(this);
	if (Lock()) {
		SetupJobList();
		Unlock();
	}
}


Folder::~Folder() {
	if (!Lock()) return;
		// release jobs
	for (int i = 0; i < fJobs.CountItems(); i ++) {
		Job* job = fJobs.ItemAt(i);
		job->Release();
	}
	Unlock();
}

Job* Folder::GetNextJob() {
	for (int i = 0; i < fJobs.CountItems(); i ++) {
		Job* job = fJobs.ItemAt(i);
		if (job->IsValid() && job->IsWaiting()) {
			job->Acquire(); return job;
		}
	}
	return NULL;
}

