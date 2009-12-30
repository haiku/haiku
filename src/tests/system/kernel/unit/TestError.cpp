/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TestError.h"

#include <stdlib.h>


TestError::TestError(Test* test, char* message)
	:
	fTest(test),
	fMessage(message)
{
}


TestError::~TestError()
{
	free(fMessage);
}
