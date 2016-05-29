/*
 * Copyright 2009-2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUGGER_GLOBALS_H
#define DEBUGGER_GLOBALS_H

#include "TargetHostInterfaceRoster.h"


status_t debugger_global_init(TargetHostInterfaceRoster::Listener* listener);

void debugger_global_uninit();

#endif // DEBUGGER_GLOBALS_H
