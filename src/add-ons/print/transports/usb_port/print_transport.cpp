/*****************************************************************************/
// Usb port transport add-on,
// changes by Andreas Benzler
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
// Copyright (c) 2001-2003 OpenBeOS Project
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


#include <unistd.h>
#include <stdio.h>

#include <StorageKit.h>
#include <SupportKit.h>

#include "TransportAddOn.h"

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
		return transport;
	} else {
		delete transport; return NULL;
	}
}


// Implementation of UsbPort

UsbPort::UsbPort(BDirectory* printer, BMessage *msg) 
	: fFile(-1)
{
	// We support only one USB printer, so does BeOS R5.
	fFile = open("/dev/printer/usb/0", O_RDWR | O_EXCL | O_BINARY, 0);
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

