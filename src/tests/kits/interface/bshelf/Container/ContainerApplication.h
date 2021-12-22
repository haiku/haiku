// ContainerApplication.h



#if !defined(CONTAINER_APPLICATION_H)
#define CONTAINER_APPLICATION_H

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <File.h>
#include <iostream.h>


#include <Path.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Application.h>

#include "ContainerWindow.h"
#include "IEApplication.h"

IEResourceHandler *resourcehandler;

class ContainerApplication : public IEApplication 
{
	public:
							ContainerApplication();
							~ContainerApplication();
		
			virtual void	MessageReceived (BMessage *msg);
			virtual void 	ReadyToRun(void);
			BEntry 				*archive_file(bool create = TRUE);	
		
	private:
			BMessenger	fMsgnr;	
			BFile				*fStream;	
			bool				fRemove;
			bool				fTest;				
			ContainerWindow *fContainerWindow;	

};

#endif 
