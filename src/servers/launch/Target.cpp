/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Target.h"


Target::Target(const char* name)
	:
	BJob(name)
{
}


const char*
Target::Name() const
{
	return Title().String();
}


status_t
Target::AddData(const char* name, BMessage& data)
{
	return fData.AddMessage(name, &data);
}


status_t
Target::Execute()
{
	return B_OK;
}
