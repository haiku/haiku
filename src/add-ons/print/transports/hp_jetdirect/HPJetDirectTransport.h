/*****************************************************************************/
// HP JetDirect (TCP/IP only) transport add-on,
//
// Author
//   Philippe Houdoin
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

#ifndef HP_JETDIRECT_TRANSPORT_H
#define HP_JETDIRECT_TRANSPORT_H

#include <Message.h>
#include <Directory.h>
#include <NetEndpoint.h>

class HPJetDirectPort : public BDataIO {	
	public:
		HPJetDirectPort(BDirectory* printer, BMessage* msg);
		~HPJetDirectPort();

		bool Ready() { return fReady; }

		ssize_t Read(void* buffer, size_t size);
		ssize_t Write(const void* buffer, size_t size);

	private:
		char fHost[256];
		uint16 fPort;		// default is 9100
		BNetEndpoint *fEndpoint;
		bool fReady;
};

#endif
