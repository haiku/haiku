#include <Path.h>
#include <Roster.h>
#include <stdio.h>

#include <FileTypeApp.h>
#include <FileTypeConstants.h>
#include <FileTypeWindow.h>

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
		const char ** argv = new (const char*)[argc];
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
FileTypeApp::RefsReceived(BMessage * message)
{
	BList entryList;
	int32 i;
	entry_ref ref;
	while (message->FindRef("refs",i++,&ref) == B_OK) {
		entryList.AddItem(new BEntry(&ref,true));
	}
	if (entryList.CountItems() == 0) {
		return;
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
	fprintf(stderr,"Usage: %s [FILES]\n",execname);
	fprintf(stderr,"Open a FileType window for the given FILES.\n");
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
		
		entry_ref ref;
		if (get_ref_for_path(path.Path(), &ref) != B_OK) {
			printf("get_ref_for_path failed: \"");
			printf("%s",path.Path());
			printf("\".\n");
			continue;
		}
		
		entryList.AddItem(new BEntry(&ref,true));
	}
	if (entryList.CountItems() == 0) {
		PrintUsage(argv[0]);
		return;
	}
	FileTypeWindow * window = new FileTypeWindow(&entryList);
	if (window->InitCheck() != B_OK) {
		printf("Failed to create FileTypeWindow\n");
		return;
	}
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
		fOpenPanel = new BFilePanel(B_OPEN_PANEL);
	}
	return fOpenPanel;
}
