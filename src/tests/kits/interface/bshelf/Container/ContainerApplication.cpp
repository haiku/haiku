// ContainerApplication.cpp


#include "ContainerApplication.h"


const char *XSHELF_INSPECTOR_APP	= "application/x-vnd.reh-XShelfInspector";	// Mime XShelfInspector
const char *BACKSTORE_PATH				= "XContainer/XContainer_data";


ContainerApplication :: ContainerApplication()
		  		  					: IEApplication("application/x-vnd.reh-XContainer")
{
	fTest 	= false;
	fStream = NULL;
	fRemove = false;	
	BEntry		*entry;
	fContainerWindow = NULL;

	if ((entry = archive_file())) 
	{
		fStream = new BFile(entry, O_RDWR);
		delete entry;
	}
	
	fContainerWindow = new ContainerWindow(fStream);
}


ContainerApplication :: ~ ContainerApplication()
{
	be_roster -> StopWatching(fMsgnr);	
	if (fStream != NULL) delete fStream;
	
	if ( (fRemove == true) && (fTest == false)	)	// delete backing store file
	{
		BPath		path;
		BPath		parent;
		if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK) return; // path: /boot/home/config/settings
		path.Append (BACKSTORE_PATH);								
		
		path.GetParent(&parent);										// lock for parent
		create_directory(parent.Path(), 0777);			// create directory
		parent.Unset();															// close
		
		BEntry	entry(path.Path());		
		entry_ref	ref;
		entry.GetRef(&ref);						
					
		BFile file;
		file.SetTo(&ref,B_ERASE_FILE);							// erase file
		file.Unset();
	}
}



BEntry  *ContainerApplication :: archive_file(bool create)
{
	BPath		path;
	BEntry	*entry = NULL;
	BPath		parent;
	
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)	// path: /boot/home/config/settings 
		return NULL;

	path.Append (BACKSTORE_PATH);	

	if (create) 
	{
		path.GetParent(&parent);											// lock for parent
		create_directory(parent.Path(), 0777);				// create directory
		parent.Unset();																// close

		int	fd;																				
		fd  = open(path.Path(), O_RDWR);
		if (fd < 0)  fd = creat(path.Path(), 0777);
		if (fd > 0)  close(fd);
	}

	entry = new BEntry(path.Path());
	if (entry->InitCheck() != B_NO_ERROR) 
	{
		delete entry;
		entry = NULL;
	}
	return entry;
}
	

void ContainerApplication::MessageReceived(BMessage *message)
{

	switch(message->what)
	{
		case CLEAR_FILE:
			{
				message -> FindBool("Remove",&fRemove);
			}
			break;	

		case TEST_REPLICANT:
			{
				message -> FindBool("Test",&fTest);
			}
			break;	

		case B_SOME_APP_ACTIVATED:
		case B_SOME_APP_LAUNCHED:
			{
				fContainerWindow -> Lock();
					bool value =  be_roster -> IsRunning(XSHELF_INSPECTOR_APP);
					fContainerWindow->EnableRemoveButton(value);									// Only when XShelf.. is running
					fContainerWindow->EnableTestButton(value);										// Buttons are enabled
				fContainerWindow -> Unlock();
			}
			break;

		case B_SOME_APP_QUIT:															// Quit XContaier when XShelfInspector is quitting
			{
				fTest = false;
				bool value =  be_roster -> IsRunning(XSHELF_INSPECTOR_APP);
				if ( !value && fContainerWindow->Lock() ) fContainerWindow -> QuitRequested();		
			}
			break;
	
		default:
			BApplication::MessageReceived(message);
			break;
	}

}

void ContainerApplication::ReadyToRun()
{		
	fMsgnr = BMessenger(this);
	be_roster -> StartWatching(fMsgnr);
	PostMessage(B_SOME_APP_LAUNCHED);
}



int main()
{	
	ContainerApplication	theApplication;

	theApplication.Run();

	return(0);
}
