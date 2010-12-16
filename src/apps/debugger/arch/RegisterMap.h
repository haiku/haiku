/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef REGISTER_MAP_H
#define REGISTER_MAP_H


#include <Referenceable.h>


class RegisterMap : public BReferenceable {
public:
	virtual						~RegisterMap();

	virtual	int32				CountRegisters() const = 0;
	virtual	int32				MapRegisterIndex(int32 index) const = 0;
};


#endif	// REGISTER_MAP_H
