/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold (bonefish@users.sf.net)
 */

//! API classes - registrar interface.


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
	return "_haiku_roster_port_";
}

}	// namespace BPrivate
