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
// Copyright (c) 2001,2002 OpenBeOS Project
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

static BList *	g_files_list 	= NULL;
static uint32	g_nb_files 		= 0;

status_t	AddFile(BFile * file);
status_t	RemoveFile();

extern "C" _EXPORT void exit_transport()
{
	printf("exit_transport\n");
	RemoveFile();
}

extern "C" _EXPORT BDataIO * init_transport
	(
	BMessage *	msg
	)
{
	BFile * 		file;
	FileSelector *	selector;
	entry_ref		ref;
	
	printf("init_transport\n");

	selector = new FileSelector();
	if (selector->Go(&ref) != B_OK)
		return NULL;

	file = new BFile(&ref, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if ( file->InitCheck() != B_OK ) {
		if (msg)
			msg->what = 'canc';	// Indicates user cancel the panel...
		delete file;
		return NULL;
	};
		
	AddFile(file);
	
	BPath path;
	path.SetTo(&ref);
	
	if (msg) {
		// Print transport add-ons should set to 'okok' the message on success
		msg->what = 'okok';	
		msg->AddString("path", path.Path()); // Add path of new choosen file to transport message
	}
	return file;
}

status_t AddFile(BFile * file)
{
	if (!file)
		return B_ERROR;
		
	if (!g_files_list)
		{
		g_files_list = new BList();
		g_nb_files = 0;
		};

	g_files_list->AddItem(file);
	g_nb_files++;

	printf("AddFile: %ld file transport(s) instanciated\n", g_nb_files);

	return B_OK;
}

status_t RemoveFile()
{
	void *	file;
	int32	i;

	g_nb_files--;

	printf("RemoveFile: %ld file transport(s) still instanciated\n", g_nb_files);

	if (g_nb_files)
		return B_OK;

	printf("RemoveFile: deleting files list...\n");

	for (i = 0; (file = g_files_list->ItemAt(i)); i++)
		delete file;		
	
	delete g_files_list;
	return B_OK;
}
