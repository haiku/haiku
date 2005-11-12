#include <Path.h>
#include <Roster.h>
#include <stdio.h>

#include <FileTypesConstants.h>
#include <FileTypesApp.h>
#include <FileTypesWindow.h>

BRect windowRect(7,26,507,426);

FileTypesApp * file_types_app = 0;

FileTypesApp::FileTypesApp()
	: BApplication(APP_SIGNATURE)
{
	fOpenPanel = new BFilePanel();
	file_types_app = this;
}

void FileTypesApp::DispatchMessage(BMessage *msg, BHandler *handler)
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
FileTypesApp::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case FILE_OPEN:
			fOpenPanel->Show();
		break;
		case B_SILENT_RELAUNCH:
			fWindow->Show();
		break;
		default:
			BApplication::MessageReceived(message);
		break;
	} 
}

void
FileTypesApp::RefsReceived(BMessage *message)
{
	int32 refNum = 0;
	entry_ref ref;
	while (message->FindRef("refs", refNum++, &ref) == B_OK) {
		
	}
}

void
FileTypesApp::ArgvReceived(int32 argc, const char *argv[], const char * cwd)
{
	BMessage refsReceived(B_REFS_RECEIVED);
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
	}
}

void 
FileTypesApp::ReadyToRun() 
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
FileTypesApp::OpenPanel()
{
	if (fOpenPanel == 0) {
		fOpenPanel = new BFilePanel(B_OPEN_PANEL);
	}
	return fOpenPanel;
}
