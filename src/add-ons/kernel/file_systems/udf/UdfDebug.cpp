//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  UDF version copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//  Initial version copyright (c) 2002 Axel Dörfler, axeld@pinc-software.de
//----------------------------------------------------------------------

/*! \file UdfDebug.cpp

	Support code for handy debugging macros.
*/

#include "UdfDebug.h"

#include <KernelExport.h>
#include <TLS.h>

//----------------------------------------------------------------------
// declarations
//----------------------------------------------------------------------

static void indent();
static void unindent();
static int32 get_tls_handle();

//! Used to keep the tls handle from being allocated more than once.
vint32 tls_spinlock = 0;

/*! \brief Used to flag whether the tls handle has been allocated yet.

	Not sure if this really needs to be \c volatile or not...
*/
volatile bool tls_handle_initialized = false;

//! The tls handle of the tls var used to store indentation info.
int32 tls_handle = 0;

//----------------------------------------------------------------------
// public functions
//----------------------------------------------------------------------

/*! \brief Returns the current debug indentation level for the
	current thread.
*/
int32
_get_debug_indent_level()
{
	return (int32)tls_get(get_tls_handle());
}
	
//----------------------------------------------------------------------
// static functions
//----------------------------------------------------------------------

/*! \brief Increases the current debug indentation level for
	the current thread by 1.
*/
void
indent()
{
	tls_set(get_tls_handle(), (void*)(_get_debug_indent_level()+1));
}

/*! \brief Decreases the current debug indentation level for
	the current thread by 1.
*/
void
unindent()
{
	tls_set(get_tls_handle(), (void*)(_get_debug_indent_level()-1));
}

/*! \brief Returns the thread local storage handle used to store
	indentation information, allocating the handle first if
	necessary.
*/
int32
get_tls_handle()
{
	// Init the tls handle if this is the first call.
	if (!tls_handle_initialized) {
		if (atomic_or(&tls_spinlock, 1) == 0) {
			// First one in gets to init
			tls_handle = tls_allocate();	
			tls_handle_initialized = true;
			atomic_and(&tls_spinlock, 0);
		} else {
			// All others must wait patiently
			while (!tls_handle_initialized) {
				snooze(1);
			}
		}
	}
	return tls_handle;
}

//----------------------------------------------------------------------
// _DebugHelper
//----------------------------------------------------------------------

/*! \brief Increases the current indentation level.
*/
_DebugHelper::_DebugHelper()
{
	indent();
}

/*! \brief Decreases the current indentation level.
*/
_DebugHelper::~_DebugHelper()
{
	unindent();
}

