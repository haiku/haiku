/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Variable.h"

#include "Type.h"
#include "ValueLocation.h"


Variable::Variable(const BString& name, Type* type, ValueLocation* location)
	:
	fName(name),
	fType(type),
	fLocation(location)
{
	fType->AcquireReference();
	fLocation->AcquireReference();
}


Variable::~Variable()
{
	fType->ReleaseReference();
	fLocation->ReleaseReference();
}
