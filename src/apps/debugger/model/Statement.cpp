/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Statement.h"


// #pragma mark - Statement


Statement::~Statement()
{
}


// #pragma mark - AbstractStatement


AbstractStatement::AbstractStatement(const SourceLocation& start,
	const SourceLocation& end)
	:
	fStart(start),
	fEnd(end)
{
}


SourceLocation
AbstractStatement::StartSourceLocation() const
{
	return fStart;
}


SourceLocation
AbstractStatement::EndSourceLocation() const
{
	return fEnd;
}


// #pragma mark - ContiguousStatement


ContiguousStatement::ContiguousStatement(const SourceLocation& start,
	const SourceLocation& end, const TargetAddressRange& range)
	:
	AbstractStatement(start, end),
	fRange(range)
{
}


TargetAddressRange
ContiguousStatement::CoveringAddressRange() const
{
	return fRange;
}


int32
ContiguousStatement::CountAddressRanges() const
{
	return 1;
}


TargetAddressRange
ContiguousStatement::AddressRangeAt(int32 index) const
{
	return index == 0 ? fRange : TargetAddressRange();
}


bool
ContiguousStatement::ContainsAddress(target_addr_t address) const
{
	return fRange.Contains(address);
}
