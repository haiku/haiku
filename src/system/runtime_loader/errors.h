/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ERRORS_H
#define ERRORS_H

#include <util/KMessage.h>

#include "runtime_loader_private.h"


extern KMessage gErrorMessage;


static inline bool
report_errors()
{
	return gProgramArgs->error_port >= 0;
}


#endif	// ERRORS_H
