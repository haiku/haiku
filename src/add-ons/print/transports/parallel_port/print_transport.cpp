/*****************************************************************************/
// Parallel port transport add-on.
//
// Author
//   Michael Pfeiffer
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001,2002 OpenBeOS Project
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

#include <StorageKit.h>
#include <SupportKit.h>

class ParallelPort : public BDataIO {
	int fFile;
	
public:
	ParallelPort(BMessage* msg);
	~ParallelPort();

	bool IsOk() { return fFile > -1; }

	ssize_t Read(void* buffer, size_t size);
	ssize_t Write(const void* buffer, size_t size);
};

// Only one connection per add-on permitted!
static ParallelPort* gPort = NULL;

// Implementation of transport add-on interface

extern "C" _EXPORT BDataIO * init_transport
	(
	BMessage *	msg
	)
{
	if (msg != NULL && gPort == NULL) {
		ParallelPort* port = new ParallelPort(msg);
		if (port->IsOk()) {
			gPort = port;
			return port;
		}
		delete port;
	}
	return NULL;
}

extern "C" _EXPORT void exit_transport()
{
	if (gPort != NULL) {
		delete gPort;
	}
}


// Impelmentation of ParallelPort
ParallelPort::ParallelPort(BMessage* msg) 
	: fFile(-1)
{
	const char* printer_name = msg->FindString("printer_file");
	char address[80];
	char device[B_PATH_NAME_LENGTH];

	if (printer_name && *printer_name != '\0') {
		BDirectory printer(printer_name);
		if (printer.InitCheck() != B_OK) return;
		
		int size = printer.ReadAttr("transport_address", B_STRING_TYPE, 0, address, sizeof(address));
		if (size <= 0 || size >= sizeof(address)) return;
		address[size] = 0; // make sure string is 0-terminated
		
		strcat(strcpy(device, "/dev/parallel/"), address);
		fFile = open(device, O_RDWR | O_EXCL | O_BINARY, 0);
	}
}

ParallelPort::~ParallelPort() {
	if (IsOk()) {
		close(fFile); fFile = -1;
	}
}

ssize_t ParallelPort::Read(void* buffer, size_t size) {
	return read(fFile, buffer, size);
}

ssize_t ParallelPort::Write(const void* buffer, size_t size) {
	return write(fFile, buffer, size);
}

