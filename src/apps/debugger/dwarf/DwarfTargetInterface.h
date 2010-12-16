/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_TARGET_INTERFACE_H
#define DWARF_TARGET_INTERFACE_H


#include <Referenceable.h>
#include <Variant.h>

#include "Types.h"


class CfaContext;
class Register;


class DwarfTargetInterface : public BReferenceable {
public:
	virtual						~DwarfTargetInterface();

	virtual	uint32				CountRegisters() const = 0;
	virtual	uint32				RegisterValueType(uint32 index) const = 0;

	virtual	bool				GetRegisterValue(uint32 index,
									BVariant& _value) const = 0;
	virtual	bool				SetRegisterValue(uint32 index,
									const BVariant& value) = 0;
	virtual	bool				IsCalleePreservedRegister(uint32 index) const
									= 0;
	virtual status_t			InitRegisterRules(CfaContext& context) const
									= 0;

	virtual	bool				ReadMemory(target_addr_t address, void* buffer,
									size_t size) const = 0;
	virtual	bool				ReadValueFromMemory(target_addr_t address,
									uint32 valueType, BVariant& _value) const
										= 0;
	virtual	bool				ReadValueFromMemory(target_addr_t addressSpace,
									target_addr_t address,
									uint32 valueType, BVariant& _value) const
										= 0;
};


#endif	// DWARF_TARGET_INTERFACE_H
