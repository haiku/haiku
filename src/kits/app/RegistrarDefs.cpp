//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		RegistrarDefs.cpp
//	Author(s):		Ingo Weinhold (bonefish@users.sf.net)
//	Description:	API classes - registrar interface.
//------------------------------------------------------------------------------

#include <AppMisc.h>
#include <RegistrarDefs.h>

namespace BPrivate {

// names
const char *kRegistrarSignature	= "application/x-vnd.haiku-registrar";
const char *kRosterThreadName	= "_roster_thread_";
const char *kRAppLooperPortName	= "rAppLooperPort";

// get_roster_port_name
/*!	\brief Returns the name of the main request port of the registrar (roster).
	\return the name of the registrar request port.
*/
const char *
get_roster_port_name()
{
	return (BPrivate::is_running_on_haiku()
		? "_roster_port_" : "_obos_roster_port_");
}

}	// namespace BPrivate
