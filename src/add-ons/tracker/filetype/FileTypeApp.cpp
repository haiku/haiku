#include <stdio.h>
#include <string.h>

#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>

#include "AppTypeWindow.h"
#include "FileTypeApp.h"
#include "FileTypeConstants.h"
#include "FileTypeWindow.h"

FileTypeApp * file_type_app = 0;

FileTypeApp::FileTypeApp()
	: BApplication(APP_SIGNATURE)
{
	file_type_app = this;
	fArgvOkay = true;
}

void FileTypeApp::DispatchMessage(BMessage * msg, BHandler * handler)
{
	if ( msg->what == B_ARGV_RECEIVED ) {
		int32 argc;
		if (msg->FindInt32("argc",&argc) != B_OK) {
			argc=0;
		}
		const char ** argv = new const char*[argc];
		for (int arg = 0; (arg < argc) ; arg++) {
			if (msg->FindString("argv",arg,&argv[arg]) != B_OK) {
				argv[arg] = "";
			}
		}
		const char * cwd;
		if (msg->FindString("cwd",&cwd) != B_OK) {
			cwd = "";
		}
		ArgvReceived(argc, argv, cwd);
	} else {
		BApplication::DispatchMessage(msg,handler);
	}
}

void
FileTypeApp::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case B_CANCEL:
			if (fWindow == 0) {
				Quit();
			}			
		break;
		default:
			BApplication::MessageReceived(message);
		break;
	} 
}

void
FileTypeApp::RefsReceived(BMessage * message)
{
	BList entryList;
	int32 i = 0;
	entry_ref ref;
	while (message->FindRef("refs",i++,&ref) == B_OK) {
		BEntry * entry = new BEntry(&ref,true);
		if (!entry || (entry->InitCheck() != B_OK) || (!entry->Exists())) {
			// ignore bogus refs
			delete entry;
			continue;
		}
		entryList.AddItem(entry);
	}
	if (entryList.CountItems() == 0) {
		return;
	}
	if (entryList.CountItems() == 1) {
		BEntry * entry = static_cast<BEntry*>(entryList.FirstItem()); 
		BNode node(entry);
		if (node.InitCheck() != B_OK) {
			delete entry;
			return;
		}
		BNodeInfo nodeInfo(&node);
		if (nodeInfo.InitCheck() != B_OK) {
			delete entry;
			return;
		}
		char string[MAXPATHLEN];
		if ((nodeInfo.GetType(string) == B_OK)
		    && (strcmp(string,"application/x-vnd.Be-elfexecutable") == 0)) {
		    AppTypeWindow * window = new AppTypeWindow(entry);
		    if (window->InitCheck() == B_OK) {
		    	fWindow = window;
		    }
		    return;
		}
	}
	FileTypeWindow * window = new FileTypeWindow(&entryList);
	if (window->InitCheck() == B_OK) {
		fWindow = window;
	}
}

void
FileTypeApp::PrintUsage(const char * execname) {
	if (execname == 0) {
		execname = "FileType";
	}
	fprintf(stderr,"Usage: %s [OPTIONS] [FILES]\n",execname);
	fprintf(stderr,"Open a FileType window for the given FILES.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"  -h, --help        print this help\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Report bugs to shatty@myrealbox.com\n");
	fprintf(stderr,"\n");
}

void
FileTypeApp::ArgvReceived(int32 argc, const char * argv[], const char * cwd)
{
	fArgvOkay = false;
	BList entryList;
	for (int i = 1 ; (i < argc) ; i++) {
		BPath path;
		if (argv[i][0] == '/') {
			path.SetTo(argv[i]);
		} else {
			path.SetTo(cwd,argv[i]);
		}
		if (path.InitCheck() != B_OK) {
			printf("path.InitCheck failed: \"");
			if (argv[i][0] == '/') {
				printf("%s",argv[i]);
			} else {
				printf("%s/%s",cwd,argv[i]);
			}
			printf("\".\n");
			continue;
		}
		
		BEntry * entry = new BEntry(path.Path(),true);
		if (!entry || (entry->InitCheck() != B_OK)) {
			printf("failed to allocate BEntry: \"");
			if (argv[i][0] == '/') {
				printf("%s",argv[i]);
			} else {
				printf("%s/%s",cwd,argv[i]);
			}
			printf("\".\n");
			delete entry;
			continue;
		}
		
		if (!entry->Exists()) {
			if ((strcmp(argv[i],"-h") == 0) ||
			    (strcmp(argv[i],"-H") == 0) ||
			    (strcmp(argv[i],"-help") == 0) ||
			    (strcmp(argv[i],"--help") == 0)) {
				for (int32 i = 0 ; (i < entryList.CountItems()) ; i++) {
					delete static_cast<BEntry*>(entryList.ItemAt(i));
				}
				entryList.MakeEmpty();
				delete entry;
				break;
			} else {
				printf("file does not exist: \"");
				if (argv[i][0] == '/') {
					printf("%s",argv[i]);
				} else {
					printf("%s/%s",cwd,argv[i]);
				}
				printf("\".\n");
				delete entry;
				continue;
			}
		}
		
		entryList.AddItem(entry);
	}
	if (entryList.CountItems() == 0) {
		PrintUsage(argv[0]);
		return;
	}
	if (entryList.CountItems() == 1) {
		BEntry * entry = static_cast<BEntry*>(entryList.FirstItem()); 
		BNode node(entry);
		if (node.InitCheck() != B_OK) {
			delete entry;
			return;
		}
		BNodeInfo nodeInfo(&node);
		if (nodeInfo.InitCheck() != B_OK) {
			delete entry;
			return;
		}
		char string[MAXPATHLEN];
		if ((nodeInfo.GetType(string) == B_OK)
		    && (strcmp(string,"application/x-vnd.Be-elfexecutable") == 0)) {
		    AppTypeWindow * window = new AppTypeWindow(entry);
		    if (window->InitCheck() == B_OK) {
		    	fWindow = window;
				fArgvOkay = true;
		    } else {
		    	printf("Failed to create AppTypeWindow\n");
		    }
		    return;
		}
	}
	FileTypeWindow * window = new FileTypeWindow(&entryList);
	if (window->InitCheck() != B_OK) {
		printf("Failed to create FileTypeWindow\n");
		return;
	}
	fWindow = window;
	fArgvOkay = true;
}

void 
FileTypeApp::ReadyToRun() 
{
	if (!fArgvOkay) {
		Quit();
		return;
	}
	if (fWindow == 0) {
		OpenPanel()->Show();
	}
}

BFilePanel *
FileTypeApp::OpenPanel()
{
	if (fOpenPanel == 0) {
		fOpenPanel = new BFilePanel(B_OPEN_PANEL,NULL,NULL,B_FILE_NODE|B_DIRECTORY_NODE);
	}
	return fOpenPanel;
}
