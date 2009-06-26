/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_MEMORY_H
#define TEAM_MEMORY_H

#include "TargetAddressRange.h"


class TeamMemory {
public:
	virtual						~TeamMemory();


	virtual	ssize_t				ReadMemory(target_addr_t address, void* buffer,
									size_t size) = 0;
};


#endif	// TEAM_MEMORY_H
