#include "AutoRaiseApp.h"
#include "AutoRaiseIcon.h"
#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AutoRaiseApp"


AutoRaiseApp::AutoRaiseApp()
	: BApplication( APP_SIG )
{
	B_TRANSLATE_MARK_SYSTEM_NAME_VOID("AutoRaise");

	removeFromDeskbar(NULL);
	fPersist = true;
	fDone = false;
}

AutoRaiseApp::~AutoRaiseApp()
{
	return;
}

void AutoRaiseApp::ArgvReceived(int32 argc, char ** argv)
{
	BString option;

	for (int32 i = 1; i < argc; i++)
	{
		option = argv[i];
		if (option.IFindFirst("deskbar") != B_ERROR)
			fPersist = false;

		if (option.IFindFirst("persist") != B_ERROR)
			fPersist = true;

		if (option.IFindFirst("-h") != B_ERROR
			|| option.IFindFirst("help") != B_ERROR) {
			BString usageNote = 
				"\nUsage: " APP_NAME " [options]\n\t--deskbar\twill not open "
				"window, will just put " APP_NAME " into tray\n\t--persist (default) will put "
				APP_NAME " into tray such that it remains across reboots";
			puts(usageNote.String());
			fDone = true;
			be_app_messenger.SendMessage(B_QUIT_REQUESTED);
		}
	}
}

void AutoRaiseApp::ReadyToRun()
{
	if (!fDone)
		PutInTray(fPersist);
	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
}

void AutoRaiseApp::PutInTray(bool persist)
{
	BDeskbar db;

	if (!persist)
		db.AddItem(new TrayView);
	else {
		BRoster roster;
		entry_ref ref;
		roster.FindApp(APP_SIG, &ref);
		int32 id;
		db.AddItem(&ref, &id);
	}
}

int main()
{
	AutoRaiseApp *app = new AutoRaiseApp();
	app->Run();
}
