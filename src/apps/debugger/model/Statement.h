/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STATEMENT_H
#define STATEMENT_H

#include <Referenceable.h>

#include "SourceLocation.h"
#include "TargetAddressRange.h"
#include "Types.h"


class Statement : public BReferenceable {
public:
	virtual						~Statement();

	virtual	SourceLocation		StartSourceLocation() const = 0;

	virtual	TargetAddressRange	CoveringAddressRange() const = 0;

	virtual	int32				CountAddressRanges() const = 0;
	virtual	TargetAddressRange	AddressRangeAt(int32 index) const = 0;

	virtual	bool				ContainsAddress(target_addr_t address)
									const = 0;
};


class AbstractStatement : public Statement {
public:
								AbstractStatement(const SourceLocation& start);

	virtual	SourceLocation		StartSourceLocation() const;

protected:
			SourceLocation		fStart;
};


class ContiguousStatement : public AbstractStatement {
public:
								ContiguousStatement(const SourceLocation& start,
									const TargetAddressRange& range);

			const TargetAddressRange& AddressRange() const
										{ return fRange; }

	virtual	TargetAddressRange	CoveringAddressRange() const;

	virtual	int32				CountAddressRanges() const;
	virtual	TargetAddressRange	AddressRangeAt(int32 index) const;

	virtual	bool				ContainsAddress(target_addr_t address) const;

protected:
			TargetAddressRange	fRange;
};


#endif	// STATEMENT_H
