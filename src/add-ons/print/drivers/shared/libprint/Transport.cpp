/*
 * Transport.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <FindDirectory.h>
#include <Message.h>
#include <Directory.h>
#include <DataIO.h>
#include <File.h>
#include <Path.h>
#include <image.h>

#include "Transport.h"
#include "PrinterData.h"
#include "DbgMsg.h"

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

Transport::Transport(const PrinterData *printer_data)
	: fImage(-1), fInitTransport(0), fExitTransport(0), fDataStream(0), fAbort(false)
{
	BPath path;

	if (B_OK == find_directory(B_USER_ADDONS_DIRECTORY, &path)) {
		path.Append("Print/transport");
		path.Append(printer_data->getTransport().c_str());
		DBGMSG(("load_add_on: %s\n", path.Path()));
		fImage = load_add_on(path.Path());
	}
	if (fImage < 0) {
		if (B_OK == find_directory(B_BEOS_ADDONS_DIRECTORY, &path)) {
			path.Append("Print/transport");
			path.Append(printer_data->getTransport().c_str());
			DBGMSG(("load_add_on: %s\n", path.Path()));
			fImage = load_add_on(path.Path());
		}
	}

	if (fImage < 0) {
		set_last_error("cannot load a transport add-on");
		return;
	}

	DBGMSG(("image id = %d\n", (int)fImage));

	get_image_symbol(fImage, "init_transport", B_SYMBOL_TYPE_TEXT, (void **)&fInitTransport);
	get_image_symbol(fImage, "exit_transport", B_SYMBOL_TYPE_TEXT, (void **)&fExitTransport);

	if (fInitTransport == NULL) {
		set_last_error("get_image_symbol failed.");
		DBGMSG(("init_transport is NULL\n"));
	}

	if (fExitTransport == NULL) {
		set_last_error("get_image_symbol failed.");
		DBGMSG(("exit_transport is NULL\n"));
	}

	if (fInitTransport) {
		char spool_path[256];
		printer_data->getPath(spool_path);
		BMessage *msg = new BMessage('TRIN');
		msg->AddString("printer_file", spool_path);
		fDataStream = (*fInitTransport)(msg);
		delete msg;
		if (fDataStream == 0) {
			set_last_error("init_transport failed.");
		}
	}
}

Transport::~Transport()
{
	if (fExitTransport) {
		(*fExitTransport)();
	}
	if (fImage >= 0) {
		unload_add_on(fImage);
	}
}

bool Transport::check_abort() const
{
	return fDataStream == 0;
}

const string &Transport::last_error() const
{
	return fLastErrorString;
}

bool Transport::is_print_to_file_canceled() const
{
	// The BeOS "Print To File" transport add-on returns a non-NULL BDataIO *
	// even after user filepanel cancellation!
	BFile* file = dynamic_cast<BFile*>(fDataStream);
	return file != NULL && file->InitCheck() != B_OK;
}

void Transport::set_last_error(const char *e)
{
	fLastErrorString = e;
	fAbort = true;
}

void Transport::write(const void *buffer, size_t size) throw(TransportException)
{
	if (fDataStream) {
		if (size == (size_t)fDataStream->Write(buffer, size)) {
			return;
		}
		set_last_error("BDataIO::Write failed.");
	}
	throw TransportException(last_error());
}
