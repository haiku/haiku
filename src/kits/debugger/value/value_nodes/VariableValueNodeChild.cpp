/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "VariableValueNodeChild.h"

#include "Variable.h"
#include "ValueLocation.h"


VariableValueNodeChild::VariableValueNodeChild(Variable* variable)
	:
	fVariable(variable)
{
	fVariable->AcquireReference();
	SetLocation(fVariable->Location(), B_OK);
}


VariableValueNodeChild::~VariableValueNodeChild()
{
	fVariable->ReleaseReference();
}


const BString&
VariableValueNodeChild::Name() const
{
	return fVariable->Name();
}


Type*
VariableValueNodeChild::GetType() const
{
	return fVariable->GetType();
}


ValueNode*
VariableValueNodeChild::Parent() const
{
	return NULL;
}


status_t
VariableValueNodeChild::ResolveLocation(ValueLoader* valueLoader,
	ValueLocation*& _location)
{
	_location = fVariable->Location();
	_location->AcquireReference();
	return B_OK;
}
