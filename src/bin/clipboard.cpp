/*
 * Copyright 2005, Haiku Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 *		Axel Dörfler, axeld@pinc-software.de.
 */


#include <Application.h>
#include <Clipboard.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>


extern const char *__progname;
static const char *kProgramName = __progname;

static int sExitValue = EXIT_SUCCESS;


class ClipboardApp : public BApplication {
	public:
		ClipboardApp(void);
		virtual ~ClipboardApp(void);

		virtual void ReadyToRun(void);
		virtual void ArgvReceived(int32 argc, char **argv);

	private:
		void Usage(void);
		status_t Load(const char *path);
		status_t Save(const char *path);
		status_t Copy(const char *string);
		status_t Clear(void);
		status_t Print(bool debug);

		bool fGotArguments;
};


ClipboardApp::ClipboardApp(void)
	: BApplication("application/x-vnd.haiku-clipboard"),
	fGotArguments(false)
{
}


ClipboardApp::~ClipboardApp(void)
{
}


void 
ClipboardApp::ArgvReceived(int32 argc, char **argv)
{
	status_t status = B_OK;

	static struct option const kLongOptions[] = {
		{"load", required_argument, 0, 'l'},
		{"save", required_argument, 0, 's'},
		{"copy", required_argument, 0, 'c'},
		{"clear", no_argument, 0, 'r'},
		{"stdin", no_argument, 0, 'i'},
		{"print", no_argument, 0, 'p'},
		{"debug", no_argument, 0, 'd'},
		{"help", no_argument, 0, 'h'},
		{NULL}
	};

	int c;
	while ((c = getopt_long(argc, argv, "l:s:o:c:ripdh", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 'l':
				status = Load(optarg);
				break;
			case 's':
			case 'o':
				status = Save(optarg);
				break;
			case 'c':
				status = Copy(optarg);
				break;
			case 'r':
				status = Clear();
				break;
			case 'i':
				status = Copy(NULL);
				break;
			case 'p':
				status = Print(false);
				break;
			case 'd':
				status = Print(true);
				break;
			case 0:
				break;
			case 'h':
				// help text is printed in ReadyToRun()
				return;
			default:
				status = B_ERROR;
				break;
		}
	}

	if (status == B_OK)
		fGotArguments =	true;
	else
		sExitValue = EXIT_FAILURE;
}

void
ClipboardApp::ReadyToRun(void)
{
	if (fGotArguments == false)
		Usage();

	PostMessage(B_QUIT_REQUESTED);
}


void
ClipboardApp::Usage(void)
{
	printf("usage: %s [-olcirpd]\n"
		"  -s, --save=file\tSave clipboard to file (flattened BMessage)\n"
		"  -o\t\t\tthe same\n" // ToDo: (\"-\" for standard output/input)\n"
		"  -l, --load=file\tLoad clipboard from file (flattened BMessage)\n"
		"  -c, --copy=string\tPut the given string on the clipboard\n"
		"  -i, --stdin\t\tLoad clipboard with string from standard input\n\n"

		"  -r, --clear\t\tRemove all contents from clipboard\n\n"

		"  -p, --print\t\tPrint clipboard to standard output\n"
		"  -d, --debug\t\tPrint clipboard message to stdout\n\n"

		"  -h, --help\t\tDisplay this help and exit\n",
		kProgramName);
}


status_t
ClipboardApp::Load(const char *path)
{
	status_t status = B_OK;
	BFile file;

	status = file.SetTo(path, B_READ_ONLY);
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not open file: %s\n", kProgramName, strerror(status));
		return status;
	}

	// get BMessage from file
	BMessage clipFromFile;
	status = clipFromFile.Unflatten(&file);
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not read clip: %s\n", kProgramName, strerror(status));
		return status;
	}

	// load clip into clipboard
	if (!be_clipboard->Lock()) {
		fprintf(stderr, "%s: could not lock clipboard.\n", kProgramName);
		return B_ERROR;
	}

	be_clipboard->Clear();

	BMessage *clip = be_clipboard->Data();
	*clip = clipFromFile;

	status = be_clipboard->Commit();
	if (status != B_OK) {
		fprintf(stderr, "%s: could not commit data to clipboard.\n", kProgramName);
		be_clipboard->Unlock();
		return status;
	}

	be_clipboard->Unlock();
	return B_OK;
}


status_t
ClipboardApp::Save(const char *path)
{
	status_t status = B_OK;
	BFile file;

	status = file.SetTo(path, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not create file: %s\n", kProgramName, strerror(status));
		return status;
	}

	// get clip
	if (!be_clipboard->Lock()) {
		fprintf(stderr, "%s: could not lock clipboard.\n", kProgramName);
		return B_ERROR;
	}

	BMessage *clip = be_clipboard->Data();

	be_clipboard->Unlock();

	// flatten clip to file
	status = clip->Flatten(&file);
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not write data: %s\n", kProgramName, strerror(status));
		return status;
	}

	return B_OK;
}


status_t
ClipboardApp::Copy(const char *string)
{
	status_t status = B_OK;
	BString inputString;

	if (string == NULL) {
		// read from standard input
		char c;
		while ((c = fgetc(stdin)) != EOF) {
			inputString += c;
		}

		string = inputString.String();
	}

	// get clipboard BMessage
	
	if (be_clipboard->Lock()) {
		be_clipboard->Clear();

		BMessage *clip = be_clipboard->Data();

		// add data to clipboard
		clip->AddData("text/plain", B_MIME_TYPE, string, strlen(string));			

		status = be_clipboard->Commit();
		if (status != B_OK) {
			fprintf(stderr, "%s: could not commit data to clipboard.\n", kProgramName);
			be_clipboard->Unlock();
			return status;
		}

		be_clipboard->Unlock();
	} else {
		fprintf(stderr, "%s: could not lock clipboard.\n", kProgramName);
		return B_ERROR;
	}

	return B_OK;
}


status_t
ClipboardApp::Clear(void)
{
	status_t status = B_OK;

	if (!be_clipboard->Lock()) {
		fprintf(stderr, "%s: could not lock clipboard.\n", kProgramName);
		return B_ERROR;
	}

	be_clipboard->Clear();

	status = be_clipboard->Commit();
	if (status != B_OK) {
		fprintf(stderr, "%s: could not clear clipboard.\n", kProgramName);
		be_clipboard->Unlock();
		return status;
	}

	be_clipboard->Unlock();
	return B_OK;
}


status_t
ClipboardApp::Print(bool debug)
{
	// get clip & print contents 
	// (or the entire clip, in case of --debug)

	if (!be_clipboard->Lock()) {
		fprintf(stderr, "%s: could not lock clipboard.\n", kProgramName);
		return B_ERROR;
	}

	BMessage *clip = be_clipboard->Data();

	if (debug) {
		clip->PrintToStream();
	} else {
		const char * textBuffer;
		int32 textLength;
		clip->FindData("text/plain", B_MIME_TYPE, (const void **)&textBuffer, &textLength);

		if (textBuffer != NULL && textLength > 0) {
			BString textString(textBuffer, textLength);
			printf("%s\n", textString.String());
		}
	}	

	be_clipboard->Unlock();
	return B_OK;
}


//	#pragma mark -


int
main()
{
	ClipboardApp clip_app;
	clip_app.Run();

	return sExitValue;
}

