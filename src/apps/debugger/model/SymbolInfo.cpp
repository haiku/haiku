/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "SymbolInfo.h"


SymbolInfo::SymbolInfo(target_addr_t address, target_size_t size, uint32 type,
	const BString& name)
	:
	fAddress(address),
	fSize(size),
	fType(type),
	fName(name)
{
}


SymbolInfo::~SymbolInfo()
{
}
