/*
 * Transport.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <FindDirectory.h>
#include <Message.h>
#include <Directory.h>
#include <DataIO.h>
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
	: __image(-1), __init_transport(0), __exit_transport(0), __data_stream(0), __abort(false)
{
	BPath path;

	if (B_OK == find_directory(B_USER_ADDONS_DIRECTORY, &path)) {
		path.Append("Print/transport");
		path.Append(printer_data->getTransport().c_str());
		DBGMSG(("load_add_on: %s\n", path.Path()));
		__image = load_add_on(path.Path());
	}
	if (__image < 0) {
		if (B_OK == find_directory(B_BEOS_ADDONS_DIRECTORY, &path)) {
			path.Append("Print/transport");
			path.Append(printer_data->getTransport().c_str());
			DBGMSG(("load_add_on: %s\n", path.Path()));
			__image = load_add_on(path.Path());
		}
	}

	if (__image < 0) {
		set_last_error("cannot load a transport add-on");
		return;
	}

	DBGMSG(("image id = %d\n", (int)__image));

	get_image_symbol(__image, "init_transport", B_SYMBOL_TYPE_TEXT, (void **)&__init_transport);
	get_image_symbol(__image, "exit_transport", B_SYMBOL_TYPE_TEXT, (void **)&__exit_transport);

	if (__init_transport == NULL) {
		set_last_error("get_image_symbol failed.");
		DBGMSG(("init_transport is NULL\n"));
	}

	if (__exit_transport == NULL) {
		set_last_error("get_image_symbol failed.");
		DBGMSG(("exit_transport is NULL\n"));
	}

	if (__init_transport) {
		char spool_path[256];
		printer_data->getPath(spool_path);
		BMessage *msg = new BMessage('TRIN');
		msg->AddString("printer_file", spool_path);
		__data_stream = (*__init_transport)(msg);
		delete msg;
		if (__data_stream == 0) {
			set_last_error("init_transport failed.");
		}
	}
}

Transport::~Transport()
{
	if (__exit_transport) {
		(*__exit_transport)();
	}
	if (__image >= 0) {
		unload_add_on(__image);
	}
}

bool Transport::check_abort() const
{
	return __data_stream == 0;
}

const string &Transport::last_error() const
{
	return __last_error_string;
}

void Transport::set_last_error(const char *e)
{
	__last_error_string = e;
	__abort = true;
}

void Transport::write(const void *buffer, size_t size) throw(TransportException)
{
	if (__data_stream) {
		if (size == (size_t)__data_stream->Write(buffer, size)) {
			return;
		}
		set_last_error("BDataIO::Write failed.");
	}
	throw TransportException(last_error());
}
