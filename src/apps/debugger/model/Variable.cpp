/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Variable.h"

#include "CpuState.h"
#include "ObjectID.h"
#include "Type.h"
#include "ValueLocation.h"


Variable::Variable(ObjectID* id, const BString& name, Type* type,
	ValueLocation* location, CpuState* state)
	:
	fID(id),
	fName(name),
	fType(type),
	fLocation(location),
	fCpuState(state)
{
	fID->AcquireReference();
	fType->AcquireReference();
	fLocation->AcquireReference();
	if (fCpuState != NULL)
		fCpuState->AcquireReference();
}


Variable::~Variable()
{
	fID->ReleaseReference();
	fType->ReleaseReference();
	fLocation->ReleaseReference();
	if (fCpuState != NULL)
		fCpuState->ReleaseReference();
}
