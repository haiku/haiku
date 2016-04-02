/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "TargetHostInterface.h"


// #pragma mark - TargetHostInterface

TargetHostInterface::~TargetHostInterface()
{
}


void
TargetHostInterface::SetName(const BString& name)
{
	fName = name;
}
