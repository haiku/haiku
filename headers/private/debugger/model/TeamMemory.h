/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_MEMORY_H
#define TEAM_MEMORY_H


#include <Referenceable.h>

#include "TargetAddressRange.h"


class BString;


class TeamMemory : public BReferenceable {
public:
	virtual						~TeamMemory();


	virtual	status_t			GetMemoryProperties(target_addr_t baseAddress,
									uint32& protection, uint32& locking) = 0;

	virtual	ssize_t				ReadMemory(target_addr_t address, void* buffer,
									size_t size) = 0;
	virtual	status_t			ReadMemoryString(target_addr_t address,
									size_t maxLength, BString& _string);
	virtual ssize_t				WriteMemory(target_addr_t address,
									void* buffer, size_t size) = 0;
};


#endif	// TEAM_MEMORY_H
