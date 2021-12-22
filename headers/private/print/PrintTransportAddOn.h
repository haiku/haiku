/*

PrintTransportAddOn

Copyright (c) 2003, 04 Haiku.

Author:
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

#ifndef _PRINT_TRANSPORT_ADD_ON_H
#define _PRINT_TRANSPORT_ADD_ON_H

#include <image.h>
#include <DataIO.h>
#include <Directory.h>
#include <Message.h>

#define B_TRANSPORT_LIST_PORTS_SYMBOL	"list_transport_ports"
#define B_TRANSPORT_FEATURES_SYMBOL	"transport_features"

// Bit values for 'transport_features'
enum {
	B_TRANSPORT_IS_HOTPLUG	= 1,
	B_TRANSPORT_IS_NETWORK	= 2,
};

// to be implemented by the transport add-on
extern "C" BDataIO* instantiate_transport(BDirectory* printer, BMessage* msg);
extern "C" status_t list_transport_ports(BMessage* msg);

#endif

