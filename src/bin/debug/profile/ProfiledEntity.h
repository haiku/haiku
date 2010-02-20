/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PROFILED_ENTITY_H
#define PROFILED_ENTITY_H


#include <SupportDefs.h>


class ProfiledEntity {
public:
	virtual						~ProfiledEntity();

	virtual	int32				EntityID() const = 0;
	virtual	const char*			EntityName() const = 0;
	virtual	const char*			EntityType() const = 0;
};


#endif	// PROFILED_ENTITY_H
