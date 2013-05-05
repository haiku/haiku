/*
 * filepanel.cpp - a command line tool to open a BFilePanel and get the result
 * copyright 2003, Francois Revol, revol@free.fr
 * LDFLAGS="-lbe -ltracker" make filepanel
 * return:
 * 0: the user has selected something,
 * 1: the user canceled/closed the panel,
 * 2: an error occured.
 */

//#define USE_FNMATCH

#ifdef USE_FNMATCH
#include <fnmatch.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <Application.h>
#include <Catalog.h>
#include <Locale.h>
#include <Messenger.h>
#include <Window.h>
#include <storage/Entry.h>
#include <storage/FilePanel.h>
#include <storage/Path.h>

#define APP_SIG "application/x-vnd.mmu_man.filepanel"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FilePanelApp"


volatile int return_code = 0;

class FilePanelApp : public BApplication
{
public:
	FilePanelApp();
	
	virtual void MessageReceived(BMessage *message);
	virtual void RefsReceived(BMessage *message);
};


FilePanelApp::FilePanelApp()
	:BApplication(APP_SIG)
{
}


void 
FilePanelApp::MessageReceived(BMessage *message)
{
	entry_ref e;
	const char *name;
	BEntry entry;
	BPath p;

	//message->PrintToStream();
	switch (message->what) {
	case B_SAVE_REQUESTED:
		message->FindRef("directory", &e);
		message->FindString("name", &name);
		entry.SetTo(&e);
		entry.GetPath(&p);
		printf("%s/%s\n", p.Path(), name);
		be_app_messenger.SendMessage(B_QUIT_REQUESTED);
		break;
	case B_CANCEL:
		return_code = 1;
		be_app_messenger.SendMessage(B_QUIT_REQUESTED);
		break;
	default:
		BApplication::MessageReceived(message);
	}
}


void 
FilePanelApp::RefsReceived(BMessage *message)
{
	entry_ref e;
	BEntry entry;
	int i;
	BPath p;
//	message->PrintToStream();
	for (i = 0; message->FindRef("refs", i, &e) == B_OK; i++) {
		entry.SetTo(&e);
		entry.GetPath(&p);
		puts(p.Path());
	}
	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
}


int
usage(char *pname, int error)
{
	fprintf(stderr, B_TRANSLATE("display a load/save file panel\n"));
	fprintf(stderr, B_TRANSLATE("usage: %s [--help] [--directory folder] "
		"[--load|--save] [--title ttl] [--single] [--modal] [--allow pattern] "
		"[--forbid pattern]\n"), pname);
	fprintf(stderr, B_TRANSLATE("usage: %s [-h]     [-d folder]              "
		"[-l|-s]     [-t ttl]      [-1]       [-m]      [-a pattern]      "
		"[-f pattern]\n"), pname);
	fprintf(stderr, B_TRANSLATE("options:\n"));
	fprintf(stderr, B_TRANSLATE("short\tlong\tdescription\n"));
	fprintf(stderr, B_TRANSLATE("-h\t--help\tdisplay usage\n"));
	fprintf(stderr, B_TRANSLATE("-d\t--directory\topen at <folder>\n"));
	fprintf(stderr, B_TRANSLATE("-l\t--load\tuse a load FilePanel "
		"(default)\n"));
	fprintf(stderr, B_TRANSLATE("-s\t--save\tuse a save FilePanel\n"));
	fprintf(stderr, B_TRANSLATE("-n\t--name\tset the default name for "
		"saving\n"));
	fprintf(stderr, B_TRANSLATE("-k\t--kind\tkind of entries that can be "
		"opened (flavour): any combination of f, d, s (file (default), "
		"directory, symlink)\n"));
	fprintf(stderr, B_TRANSLATE("-t\t--title\tset the FilePanel window "
		"title\n"));
	fprintf(stderr, B_TRANSLATE("-1\t--single\tallow only 1 file to be "
		"selected\n"));
	fprintf(stderr, B_TRANSLATE("-m\t--modal\tmakes the FilePanel modal\n"));
#ifndef USE_FNMATCH
	fprintf(stderr, B_TRANSLATE("-a\t--allow\tunimplemented\n"));
	fprintf(stderr, B_TRANSLATE("-f\t--forbid\tunimplemented\n"));
#else
	fprintf(stderr, B_TRANSLATE("-a\t--allow\tunimplemented\n"));
	fprintf(stderr, B_TRANSLATE("-f\t--forbid\tunimplemented\n"));
#endif
	return error;
}


int
main(int argc, char **argv)
{
	int i;
	file_panel_mode fpMode = B_OPEN_PANEL;
	uint32 nodeFlavour = 0;
	char *openAt = NULL;
	char *windowTitle = NULL;
	bool allowMultiSelect = true;
	bool makeModal = false;
	const char *defaultName = NULL;

	for (i = 1; i < argc; i++) {
		if (strncmp(argv[i], "--", 2) && ((*(argv[i]) == '-' && 
			strlen(argv[i]) != 2) || *(argv[i]) != '-')) {
			fprintf(stderr, B_TRANSLATE("%s not a valid option\n"), argv[i]);
			return usage(argv[0], 2);
		}
		if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			return usage(argv[0], 0);
		} else if (!strcmp(argv[i], "--directory") || !strcmp(argv[i], "-d")) {
			if (++i >= argc) {
				fprintf(stderr, B_TRANSLATE("%s: this option requires a "
					"parameter\n"), argv[i-1]);
				return usage(argv[0], 2);
			}
			openAt = argv[i];
		} else if (!strcmp(argv[i], "--load") || !strcmp(argv[i], "-l")) {
			fpMode = B_OPEN_PANEL;
		} else if (!strcmp(argv[i], "--save") || !strcmp(argv[i], "-s")) {
			fpMode = B_SAVE_PANEL;
		} else if (!strcmp(argv[i], "--name") || !strcmp(argv[i], "-n")) {
			if (++i >= argc) {
				fprintf(stderr, B_TRANSLATE("%s: this option requires a "
					"parameter\n"), argv[i-1]);
				return usage(argv[0], 2);
			}
			defaultName = (const char *)argv[i];
		} else if (!strcmp(argv[i], "--kind") || !strcmp(argv[i], "-k")) {
			if (++i >= argc) {
				fprintf(stderr, B_TRANSLATE("%s: this option requires a "
					"parameter\n"), argv[i-1]);
				return usage(argv[0], 2);
			}
			if (strchr(argv[i], 'f')) nodeFlavour |= B_FILE_NODE;
			if (strchr(argv[i], 'd')) nodeFlavour |= B_DIRECTORY_NODE;
			if (strchr(argv[i], 's')) nodeFlavour |= B_SYMLINK_NODE;
		} else if (!strcmp(argv[i], "--title") || !strcmp(argv[i], "-t")) {
			if (++i >= argc) {
				fprintf(stderr, B_TRANSLATE("%s: this option requires a "
					"parameter\n"), argv[i-1]);
				return usage(argv[0], 2);
			}
			windowTitle = argv[i];
		} else if (!strcmp(argv[i], "--single") || !strcmp(argv[i], "-1")) {
			allowMultiSelect = false;
		} else if (!strcmp(argv[i], "--modal") || !strcmp(argv[i], "-m")) {
			makeModal = true;
		} else if (!strcmp(argv[i], "--allow") || !strcmp(argv[i], "-a")) {
			if (++i >= argc) {
				fprintf(stderr, B_TRANSLATE("%s: this option requires a "
					"parameter\n"), argv[i-1]);
				return usage(argv[0], 2);
			}
			fprintf(stderr, B_TRANSLATE("%s: UNIMPLEMENTED\n"), argv[i-1]);
		} else if (!strcmp(argv[i], "--forbid") || !strcmp(argv[i], "-f")) {
			if (++i >= argc) {
				fprintf(stderr, B_TRANSLATE("%s: this option requires a "
					"parameter\n"), argv[i-1]);
				return usage(argv[0], 2);
			}
			fprintf(stderr, B_TRANSLATE("%s: UNIMPLEMENTED\n"), argv[i-1]);
		} else {
			fprintf(stderr, B_TRANSLATE("%s not a valid option\n"), argv[i]);
			return usage(argv[0], 2);
		}
	}
	new FilePanelApp;
	entry_ref panelDir;
// THIS LINE makes main() return always 0 no matter which value on return of 
// exit() ???
	BFilePanel *fPanel = new BFilePanel(fpMode, NULL, NULL, nodeFlavour, 
		allowMultiSelect, NULL, NULL, makeModal);
/**/
	if (openAt)
		fPanel->SetPanelDirectory(openAt);
	if (windowTitle)
		fPanel->Window()->SetTitle(windowTitle);
	if (fpMode == B_SAVE_PANEL && defaultName)
		fPanel->SetSaveText(defaultName);
	
	fPanel->Show();
/**/
	be_app->Run();
	delete be_app;
//	printf("rc = %d\n", return_code);
// WTF ??
//return 2;
//	exit(2);
	exit(return_code);
	return return_code;
}

