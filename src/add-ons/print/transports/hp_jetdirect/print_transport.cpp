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
// Copyright (c) 2001-2003 Haiku Project
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


#include <Message.h>
#include <Directory.h>
#include <SupportKit.h>

#include "HPJetDirectTransport.h"

static BDataIO * g_transport = NULL;

// Implementation of transport add-on interface
extern "C" _EXPORT BDataIO * init_transport(BMessage *msg)
{
	if (msg == NULL)
		return NULL;
		
	if (g_transport)
		return NULL;

	const char* printer_name = msg->FindString("printer_file");
	
	if (printer_name && *printer_name != '\0') {
		BDirectory printer(printer_name);
			
		if (printer.InitCheck() == B_OK) {
			HPJetDirectPort * transport = new HPJetDirectPort(&printer, msg);

			if (transport->InitCheck() == B_OK) {
				g_transport = transport;
				if (msg)
					msg->what = 'okok';
				return g_transport;
			};

			delete transport;
		};
	};
	return NULL;
}

extern "C" _EXPORT void exit_transport()
{
	if (g_transport)
		delete g_transport;
	g_transport = NULL;
}
