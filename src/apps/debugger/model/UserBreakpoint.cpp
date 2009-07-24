/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "UserBreakpoint.h"

#include "Function.h"
#include "FunctionID.h"
#include "LocatableFile.h"


// #pragma mark - UserBreakpointLocation


UserBreakpointLocation::UserBreakpointLocation(FunctionID* functionID,
	LocatableFile* sourceFile, const SourceLocation& sourceLocation,
	target_addr_t relativeAddress)
	:
	fFunctionID(functionID),
	fSourceFile(sourceFile),
	fSourceLocation(sourceLocation),
	fRelativeAddress(relativeAddress)
{
	fFunctionID->AcquireReference();
	if (fSourceFile != NULL)
		fSourceFile->AcquireReference();
}


UserBreakpointLocation::UserBreakpointLocation(
	const UserBreakpointLocation& other)
	:
	fFunctionID(other.fFunctionID),
	fSourceFile(other.fSourceFile),
	fSourceLocation(other.fSourceLocation),
	fRelativeAddress(other.fRelativeAddress)
{
	fFunctionID->AcquireReference();
	if (fSourceFile != NULL)
		fSourceFile->AcquireReference();
}


UserBreakpointLocation::~UserBreakpointLocation()
{
	fFunctionID->ReleaseReference();
	if (fSourceFile != NULL)
		fSourceFile->ReleaseReference();
}


UserBreakpointLocation&
UserBreakpointLocation::operator=(
	const UserBreakpointLocation& other)
{
	other.fFunctionID->AcquireReference();
	if (other.fSourceFile != NULL)
		other.fSourceFile->AcquireReference();

	fFunctionID->ReleaseReference();
	if (fSourceFile != NULL)
		fSourceFile->ReleaseReference();

	fFunctionID = other.fFunctionID;
	fSourceFile = other.fSourceFile;
	fSourceLocation = other.fSourceLocation;
	fRelativeAddress = other.fRelativeAddress;

	return *this;
}


// #pragma mark - UserBreakpointInstance


UserBreakpointInstance::UserBreakpointInstance(UserBreakpoint* userBreakpoint,
	target_addr_t address)
	:
	fAddress(address),
	fUserBreakpoint(userBreakpoint),
	fBreakpoint(NULL)
{
}


void
UserBreakpointInstance::SetBreakpoint(Breakpoint* breakpoint)
{
	fBreakpoint = breakpoint;
}


// #pragma mark - UserBreakpoint


UserBreakpoint::UserBreakpoint(const UserBreakpointLocation& location)
	:
	fLocation(location),
	fValid(false),
	fEnabled(false)
{
}


UserBreakpoint::~UserBreakpoint()
{
	for (int32 i = 0; UserBreakpointInstance* instance = fInstances.ItemAt(i);
			i++) {
		delete instance;
	}
}


int32
UserBreakpoint::CountInstances() const
{
	return fInstances.CountItems();
}


UserBreakpointInstance*
UserBreakpoint::InstanceAt(int32 index) const
{
	return fInstances.ItemAt(index);
}


bool
UserBreakpoint::AddInstance(UserBreakpointInstance* instance)
{
	return fInstances.AddItem(instance);
}


void
UserBreakpoint::RemoveInstance(UserBreakpointInstance* instance)
{
	fInstances.RemoveItem(instance);
}


UserBreakpointInstance*
UserBreakpoint::RemoveInstanceAt(int32 index)
{
	return fInstances.RemoveItemAt(index);
}


void
UserBreakpoint::SetValid(bool valid)
{
	fValid = valid;
}


void
UserBreakpoint::SetEnabled(bool enabled)
{
	fEnabled = enabled;
}
