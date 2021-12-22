/*****************************************************************************/
// Print to file transport add-on.
//
// Author
//   Philippe Houdoin
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001,2002 Haiku Project
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

#include <stdio.h>

#include <StorageKit.h>
#include <SupportKit.h>

#include "FileSelector.h"

static BList *	gFiles 	= NULL;

status_t	AddFile(BFile * file);
status_t	RemoveFile();

static const int32 kStatusOk = 'okok';
static const int32 kStatusCancel = 'canc';

extern "C" _EXPORT void exit_transport()
{
	RemoveFile();
}

extern "C" _EXPORT BDataIO * init_transport(BMessage *	msg)
{	
	FileSelector * selector = new FileSelector();
	entry_ref ref;
	if (selector->Go(&ref) != B_OK) {
		// dialog cancelled
		if (msg)
			msg->what = kStatusCancel;
		
		// for backwards compatibility with BeOS R5 return an invalid BFile
		BFile *file = new BFile();
		AddFile(file);
		return file;
	}

	BFile *file = new BFile(&ref, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if ( file->InitCheck() != B_OK ) {
		// invalid file selected
		if (msg) 
			msg->what = kStatusCancel;
		AddFile(file);
		return file;
	}
		
	if (msg) {
		// Print transport add-ons should set to 'okok' the message on success
		msg->what = kStatusOk;	
		
		BPath path;
		path.SetTo(&ref);
		msg->AddString("path", path.Path()); 
	}
	AddFile(file);
	return file;
}

status_t AddFile(BFile * file)
{
	if (file == NULL)
		return B_ERROR;
		
	if (gFiles == NULL)
		gFiles = new BList();

	gFiles->AddItem(file);

	return B_OK;
}

status_t RemoveFile()
{
	if (gFiles == NULL)
		return B_OK;

	int32 n = gFiles->CountItems();
	for (int32 i = 0; i < n; i++)
		delete (BFile*)gFiles->ItemAt(i);
	
	delete gFiles;
	gFiles = NULL;
	return B_OK;
}
