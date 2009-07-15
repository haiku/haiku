/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_EXPRESSION_EVALUATOR_H
#define DWARF_EXPRESSION_EVALUATOR_H


#include "DataReader.h"
#include "Types.h"


class DwarfTargetInterface;


class DwarfExpressionEvaluator {
public:
								DwarfExpressionEvaluator(
									DwarfTargetInterface* targetInterface,
									uint8 addressSize);
								~DwarfExpressionEvaluator();

			void				SetObjectAddress(target_addr_t address);
			void				SetFrameAddress(target_addr_t address);

			status_t			Evaluate(const void* expression, size_t size);

private:
			struct EvaluationException;

private:
	inline	void				_AssertMinStackSize(size_t size) const;

	inline	void				_Push(target_addr_t value);
	inline	target_addr_t		_Pop();

			status_t			_Evaluate();
			void				_DereferenceAddress(uint8 addressSize);
			void				_DereferenceAddressSpaceAddress(
									uint8 addressSize);
			void				_PushRegister(uint32 reg, target_addr_t offset);

private:
			DwarfTargetInterface* fTargetInterface;
			target_addr_t*		fStack;
			size_t				fStackSize;
			size_t				fStackCapacity;
			DataReader			fDataReader;
			target_addr_t		fObjectAddress;
			target_addr_t		fFrameAddress;
			uint8				fAddressSize;
			bool				fObjectAddressValid;
			bool				fFrameAddressValid;
};


#endif	// DWARF_EXPRESSION_EVALUATOR_H
