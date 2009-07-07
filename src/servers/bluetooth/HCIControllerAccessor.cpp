/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "HCIControllerAccessor.h"

HCIControllerAccessor::HCIControllerAccessor(BPath* path) : HCIDelegate(path)
{

}


HCIControllerAccessor::~HCIControllerAccessor()
{

}


status_t
HCIControllerAccessor::IssueCommand(raw_command rc, size_t size)
{

	if (Id() < 0)
		return B_ERROR;

	return B_OK;
}


status_t 
HCIControllerAccessor::Launch() {

	return B_OK;

}
