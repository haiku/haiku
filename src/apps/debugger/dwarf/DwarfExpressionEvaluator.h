/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_EXPRESSION_EVALUATOR_H
#define DWARF_EXPRESSION_EVALUATOR_H


#include "DataReader.h"
#include "Types.h"


class DwarfExpressionEvaluator {
public:
								DwarfExpressionEvaluator(uint8 addressSize);
								~DwarfExpressionEvaluator();

			status_t			Evaluate(const void* expression, size_t size);

private:
			struct EvaluationException;

private:
	inline	void				_AssertMinStackSize(size_t size) const;

	inline	void				_Push(target_addr_t value);
	inline	target_addr_t		_Pop();

			status_t			_Evaluate();

private:
			target_addr_t*		fStack;
			size_t				fStackSize;
			size_t				fStackCapacity;
			DataReader			fDataReader;
			uint8				fAddressSize;
};


#endif	// DWARF_EXPRESSION_EVALUATOR_H
