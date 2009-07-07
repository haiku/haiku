/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOURCE_CODE_H
#define SOURCE_CODE_H

#include <Referenceable.h>

#include "TargetAddressRange.h"


class Statement;


class SourceCode : public Referenceable {
public:
	virtual						~SourceCode();

	virtual	int32				CountLines() const = 0;
	virtual	const char*			LineAt(int32 index) const = 0;

	virtual	Statement*			StatementAtLine(int32 index) const = 0;
};


#endif	// SOURCE_CODE_H
