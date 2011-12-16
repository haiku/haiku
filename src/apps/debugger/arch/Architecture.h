/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H


#include <ByteOrder.h>
#include <OS.h>

#include <Referenceable.h>
#include <Variant.h>

#include "Types.h"


class CfaContext;
class CpuState;
class DisassembledCode;
class FunctionDebugInfo;
class Image;
class ImageDebugInfoProvider;
class InstructionInfo;
class Register;
class RegisterMap;
class StackFrame;
class StackTrace;
class Statement;
class Team;
class TeamMemory;


enum {
	STACK_GROWTH_DIRECTION_POSITIVE = 0,
	STACK_GROWTH_DIRECTION_NEGATIVE
};


class Architecture : public BReferenceable {
public:
								Architecture(TeamMemory* teamMemory,
									uint8 addressSize, bool bigEndian);
	virtual						~Architecture();

	virtual	status_t			Init();

	inline	uint8				AddressSize() const		{ return fAddressSize; }

	inline	bool				IsBigEndian() const		{ return fBigEndian; }
	inline	bool				IsHostEndian() const;

	virtual int32				StackGrowthDirection() const = 0;

	virtual	int32				CountRegisters() const = 0;
	virtual	const Register*		Registers() const = 0;
	virtual status_t			InitRegisterRules(CfaContext& context) const;

	virtual	status_t			GetDwarfRegisterMaps(RegisterMap** _toDwarf,
									RegisterMap** _fromDwarf) const = 0;
										// returns references

	virtual	status_t			CreateCpuState(CpuState*& _state) = 0;
	virtual	status_t			CreateCpuState(const void* cpuStateData,
									size_t size, CpuState*& _state) = 0;
	virtual	status_t			CreateStackFrame(Image* image,
									FunctionDebugInfo* function,
									CpuState* cpuState, bool isTopFrame,
									StackFrame*& _previousFrame,
									CpuState*& _previousCpuState) = 0;
										// returns reference to previous frame
										// and CPU state; returned CPU state
										// can be NULL
	virtual	void				UpdateStackFrameCpuState(
									const StackFrame* frame,
									Image* previousImage,
									FunctionDebugInfo* previousFunction,
									CpuState* previousCpuState) = 0;
										// Called after a CreateStackFrame()
										// with the image/function corresponding
										// to the CPU state.

	virtual	status_t			ReadValueFromMemory(target_addr_t address,
									uint32 valueType, BVariant& _value) const
										= 0;
	virtual	status_t			ReadValueFromMemory(target_addr_t addressSpace,
									target_addr_t address, uint32 valueType,
									BVariant& _value) const = 0;

	virtual	status_t			DisassembleCode(FunctionDebugInfo* function,
									const void* buffer, size_t bufferSize,
									DisassembledCode*& _sourceCode) = 0;
	virtual	status_t			GetStatement(FunctionDebugInfo* function,
									target_addr_t address,
									Statement*& _statement) = 0;
	virtual	status_t			GetInstructionInfo(target_addr_t address,
									InstructionInfo& _info) = 0;

			status_t			CreateStackTrace(Team* team,
									ImageDebugInfoProvider* imageInfoProvider,
									CpuState* cpuState,
									StackTrace*& _stackTrace,
									int32 maxStackDepth = -1,
									bool useExistingTrace = false);
										// team is not locked

protected:
			TeamMemory*			fTeamMemory;
			uint8				fAddressSize;
			bool				fBigEndian;
};


bool
Architecture::IsHostEndian() const
{
	return fBigEndian == (B_HOST_IS_BENDIAN != 0);
}


#endif	// ARCHITECTURE_H
