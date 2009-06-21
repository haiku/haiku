/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STATEMENT_H
#define STATEMENT_H

#include <Referenceable.h>

#include "ArchitectureTypes.h"
#include "SourceLocation.h"
#include "TargetAddressRange.h"


class Statement : public Referenceable {
public:
	virtual						~Statement();

	virtual	SourceLocation		StartSourceLocation() const = 0;
	virtual	SourceLocation		EndSourceLocation() const = 0;

	virtual	TargetAddressRange	CoveringAddressRange() const = 0;

	virtual	int32				CountAddressRanges() const = 0;
	virtual	TargetAddressRange	AddressRangeAt(int32 index) const = 0;

	virtual	bool				ContainsAddress(target_addr_t address)
									const = 0;
};


class AbstractStatement : public Statement {
public:
								AbstractStatement(const SourceLocation& start,
									const SourceLocation& end);

	virtual	SourceLocation		StartSourceLocation() const;
	virtual	SourceLocation		EndSourceLocation() const;

protected:
			SourceLocation		fStart;
			SourceLocation		fEnd;
};


class ContiguousStatement : public AbstractStatement {
public:
								ContiguousStatement(const SourceLocation& start,
									const SourceLocation& end,
									const TargetAddressRange& range);

	virtual	TargetAddressRange	CoveringAddressRange() const;

	virtual	int32				CountAddressRanges() const;
	virtual	TargetAddressRange	AddressRangeAt(int32 index) const;

	virtual	bool				ContainsAddress(target_addr_t address) const;

protected:
			TargetAddressRange	fRange;
};


#endif	// STATEMENT_H
