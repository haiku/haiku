/*
 * Copyright 2001-2015, Haiku.
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
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
const char* kRAppLooperPortName = "rAppLooperPort";
#else
const char* kRAppLooperPortName = "haiku-test:rAppLooperPort";
#endif


}	// namespace BPrivate
