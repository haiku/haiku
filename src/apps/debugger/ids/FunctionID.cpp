/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "FunctionID.h"

#include "StringUtils.h"


FunctionID::FunctionID(const BString& idString)
	:
	fIDString(idString)
{
}


FunctionID::~FunctionID()
{
}


bool
FunctionID::operator==(const ObjectID& other) const
{
	const FunctionID* functionID = dynamic_cast<const FunctionID*>(&other);
	return functionID != NULL && fIDString == functionID->fIDString;
}


uint32
FunctionID::ComputeHashValue() const
{
	return StringUtils::HashValue(fIDString);
}
