/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "TargetHostInterfaceInfo.h"


TargetHostInterfaceInfo::TargetHostInterfaceInfo(const BString& name)
	:
	BReferenceable(),
	fName(name)
{
}


TargetHostInterfaceInfo::~TargetHostInterfaceInfo()
{
}
