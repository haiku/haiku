/*****************************************************************************/
// Usb port transport add-on,
// changes by Andreas Benzler, Philippe Houdoin
//
// Original from Parallel 
// port transport add-on.
//
// Author
//   Michael Pfeiffer
// 
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001-2004 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/


#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <StorageKit.h>
#include <SupportKit.h>

#include <USB_printer.h>

#include "PrintTransportAddOn.h"

class UsbPort : public BDataIO {	
	public:
		UsbPort(BDirectory* printer, BMessage* msg);
		~UsbPort();

		bool IsOk() { return fFile > -1; }

		ssize_t Read(void* buffer, size_t size);
		ssize_t Write(const void* buffer, size_t size);

	private:
		int fFile;
};


// Implementation of transport add-on interface

BDataIO* instanciate_transport(BDirectory* printer, BMessage* msg) {
	UsbPort* transport = new UsbPort(printer, msg);
	if (transport->IsOk()) {
		msg->what = 'okok';
		return transport;
	} else {
		delete transport; return NULL;
	}
}


// Implementation of UsbPort

UsbPort::UsbPort(BDirectory* printer, BMessage *msg) 
	: fFile(-1)
{
	char device_id[USB_PRINTER_DEVICE_ID_LENGTH + 1];
	char name[USB_PRINTER_DEVICE_ID_LENGTH + 1];
	char *desc;
	char *value;
	int ret;
	bool bidirectional = true;
	
	// We support only one USB printer, so does BeOS R5.
	fFile = open("/dev/printer/usb/0", O_RDWR | O_EXCL | O_BINARY, 0);
	if (fFile < 0) {
		// Try unidirectional access mode
		bidirectional = false;
		fFile = open("/dev/printer/usb/0", O_WRONLY | O_EXCL | O_BINARY, 0);
	}
	
	if (fFile < 0)
		return;
	
	// Get printer's DEVICE ID string
	ret = ioctl(fFile, USB_PRINTER_GET_DEVICE_ID, device_id, sizeof(device_id));
	if (ret < 0) {
		close(fFile);
		fFile = -1;
		return;
	}
	
	// Fill up the message
	msg->AddBool("bidirectional", bidirectional);
	msg->AddString("device_id", device_id);

	// parse and split the device_id string into separate parameters
	desc = strtok(device_id, ":");	
	while (desc) {
		snprintf(name, sizeof(name), "DEVID:%s", desc);
		value = strtok(NULL, ";");
		if (!value)
			break;
		msg->AddString(name, value);
		
		// next device descriptor
		desc = strtok(NULL, ":");	
	}
}


UsbPort::~UsbPort()
{
	if (IsOk()) {
		close(fFile);
		fFile = -1;
	}
}


ssize_t
UsbPort::Read(void* buffer, size_t size)
{
	return read(fFile, buffer, size);
}


ssize_t
UsbPort::Write(const void* buffer, size_t size)
{
	return write(fFile, buffer, size);
}

