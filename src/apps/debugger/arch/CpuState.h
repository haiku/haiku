/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CPU_STATE_H
#define CPU_STATE_H

#include <OS.h>

#include <Referenceable.h>
#include <Variant.h>

#include "Types.h"


class Register;


class CpuState : public BReferenceable {
public:
	virtual						~CpuState();

	virtual	target_addr_t		InstructionPointer() const = 0;
	virtual target_addr_t		StackFramePointer() const = 0;
	virtual target_addr_t		StackPointer() const = 0;
	virtual	bool				GetRegisterValue(const Register* reg,
									BVariant& _value) const = 0;
	virtual	bool				SetRegisterValue(const Register* reg,
									const BVariant& value) = 0;
};


#endif	// CPU_STATE_H
