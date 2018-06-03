/*
 * Copyright 2001-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *   Michael Pfeiffer
 *   Adrien Destugues <pulkomandy@pulkomandy.tk>
 */


#include <unistd.h>
#include <stdio.h>
#include <termios.h>

#include <StorageKit.h>
#include <SupportKit.h>

#include "PrintTransportAddOn.h"


class SerialTransport : public BDataIO {
public:
	SerialTransport(BDirectory* printer, BMessage* msg);
	~SerialTransport();

	status_t InitCheck() { return fFile > -1 ? B_OK : B_ERROR; }

	ssize_t Read(void* buffer, size_t size);
	ssize_t Write(const void* buffer, size_t size);

private:
	int fFile;
};


// Impelmentation of SerialTransport
SerialTransport::SerialTransport(BDirectory* printer, BMessage* msg)
	: fFile(-1)
{
	char address[80];
	char device[B_PATH_NAME_LENGTH];
	bool bidirectional = true;

	unsigned int size = printer->ReadAttr("transport_address", B_STRING_TYPE, 0,
		address, sizeof(address));
	if (size <= 0 || size >= sizeof(address)) return;
	address[size] = 0; // make sure string is 0-terminated

	strcat(strcpy(device, "/dev/ports/"), address);
	fFile = open(device, O_RDWR | O_EXCL, 0);
	if (fFile < 0) {
		// Try unidirectional access mode
		bidirectional = false;
		fFile = open(device, O_WRONLY | O_EXCL, 0);
	}

	if (fFile < 0)
		return;

	int32 baudrate;
	size = printer->ReadAttr("transport_baudrate", B_INT32_TYPE, 0,
		&baudrate, sizeof(baudrate));

	struct termios options;
	tcgetattr(fFile, &options);

	cfmakeraw(&options);
	options.c_cc[VTIME] = 10; // wait for data at most for 1 second
	options.c_cc[VMIN] = 0; // allow to return 0 chars

	if (size == sizeof(baudrate)) {
		// Printer driver asked for a specific baudrate, configure it
		cfsetispeed(&options, baudrate);
		cfsetospeed(&options, baudrate);
	}

	tcsetattr(fFile, TCSANOW, &options);

	if (msg == NULL) {
		// Caller don't care about transport init message output content...
		return;
	}

	msg->what = 'okok';
	msg->AddBool("bidirectional", bidirectional);
	msg->AddString("_serial/DeviceName", device);

}


SerialTransport::~SerialTransport()
{
	if (InitCheck() == B_OK)
		close(fFile);
}


ssize_t
SerialTransport::Read(void* buffer, size_t size)
{
	return read(fFile, buffer, size);
}


ssize_t
SerialTransport::Write(const void* buffer, size_t size)
{
	return write(fFile, buffer, size);
}


BDataIO*
instantiate_transport(BDirectory* printer, BMessage* msg)
{
	SerialTransport* transport = new SerialTransport(printer, msg);
	if (transport->InitCheck() == B_OK)
		return transport;

	delete transport;
	return NULL;
}


status_t
list_transport_ports(BMessage* msg)
{
	BDirectory dir("/dev/ports");
	status_t rc;

	if ((rc=dir.InitCheck()) != B_OK)
		return rc;

	if ((rc=dir.Rewind()) != B_OK)
		return rc;

	entry_ref ref;
	while (dir.GetNextRef(&ref) == B_OK)
		msg->AddString("port_id", ref.name);

	return B_OK;
}
