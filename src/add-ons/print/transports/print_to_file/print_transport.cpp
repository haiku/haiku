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
	if ( file->InitCheck() != B_OK )
		{
		delete file;
		return NULL;
		};
		
	AddFile(file);
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
