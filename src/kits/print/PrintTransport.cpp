/*

PrintTransport

Copyright (c) 2004 OpenBeOS.

Authors:
	Philippe Houdoin
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include "PrintTransport.h"

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>

// implementation of class PrintTransport
PrintTransport::PrintTransport()
	: fDataIO(NULL)
	, fAddOnID(-1)
	, fExitProc(NULL)
{
}

PrintTransport::~PrintTransport()
{
	if (fExitProc) {
		(*fExitProc)();
		fExitProc = NULL;
	}

	if (fAddOnID >= 0) {
		unload_add_on(fAddOnID);
		fAddOnID = -1;
	}
}

status_t PrintTransport::Open(BNode* printerFolder)
{
	// already opened?
	if (fDataIO != NULL) {
		return B_ERROR;
	}

	// retrieve transport add-on name from printer folder attribute
	BString transportName;
	if (printerFolder->ReadAttrString("transport", &transportName) != B_OK) {
		return B_ERROR;
	}

	const directory_which paths[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY,
	};
	BPath path;
	for (uint32 i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
		if (find_directory(paths[i], &path) != B_OK)
			continue;
	path.Append("Print/transport");
	path.Append(transportName.String());
	fAddOnID = load_add_on(path.Path());
		if (fAddOnID >= 0)
			break;
	}

	if (fAddOnID < 0) {
		// failed to load transport add-on
		return B_ERROR;
	}

	// get init & exit proc
	BDataIO* (*initProc)(BMessage*);
	get_image_symbol(fAddOnID, "init_transport", B_SYMBOL_TYPE_TEXT, (void **) &initProc);
	get_image_symbol(fAddOnID, "exit_transport", B_SYMBOL_TYPE_TEXT, (void **) &fExitProc);

	if (initProc == NULL || fExitProc == NULL) {
		// transport add-on has not the proper interface
		return B_ERROR;
	}

	// now, initialize the transport add-on
	node_ref   ref;
	BDirectory dir;

	printerFolder->GetNodeRef(&ref);
	dir.SetTo(&ref);

	if (path.SetTo(&dir, NULL) != B_OK) {
		return B_ERROR;
	}

	// request BDataIO object from transport add-on
	BMessage input('TRIN');
	input.AddString("printer_file", path.Path());
	fDataIO = (*initProc)(&input);
	return B_OK;
}

BDataIO*
PrintTransport::GetDataIO()
{
	return fDataIO;
}

bool PrintTransport::IsPrintToFileCanceled() const
{
	// The BeOS "Print To File" transport add-on returns a non-NULL BDataIO *
	// even after user filepanel cancellation!
	BFile* file = dynamic_cast<BFile*>(fDataIO);
	return fDataIO == NULL || (file != NULL && file->InitCheck() != B_OK);
}
