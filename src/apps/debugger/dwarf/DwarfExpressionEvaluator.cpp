/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "DwarfExpressionEvaluator.h"

#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <new>

#include "DataReader.h"
#include "Dwarf.h"


static const size_t kMaxStackCapacityIncrement	= 64;
static const size_t kMaxStackCapacity			= 1024;


// #pragma mark - EvaluationException


struct DwarfExpressionEvaluator::EvaluationException {
};


// #pragma mark - DwarfExpressionEvaluator


void
DwarfExpressionEvaluator::_AssertMinStackSize(size_t size) const
{
	if (fStackSize < size)
		throw EvaluationException();
}


void
DwarfExpressionEvaluator::_Push(target_addr_t value)
{
	// resize the stack, if we hit the capacity
	if (fStackSize == fStackCapacity) {
		if (fStackCapacity >= kMaxStackCapacity)
			throw EvaluationException();

		size_t newCapacity = fStackCapacity + kMaxStackCapacityIncrement;
		target_addr_t* newStack = (target_addr_t*)realloc(fStack,
			newCapacity * sizeof(target_addr_t));
		if (newStack == NULL)
			throw std::bad_alloc();

		fStack = newStack;
		fStackCapacity = newCapacity;
	}

	fStack[fStackSize++] = value;
}


target_addr_t
DwarfExpressionEvaluator::_Pop()
{
	_AssertMinStackSize(1);
	return fStack[--fStackSize];
}


DwarfExpressionEvaluator::DwarfExpressionEvaluator(uint8 addressSize)
	:
	fStack(NULL),
	fStackSize(0),
	fStackCapacity(0),
	fAddressSize(addressSize)
{
}


DwarfExpressionEvaluator::~DwarfExpressionEvaluator()
{
	free(fStack);
}


status_t
DwarfExpressionEvaluator::Evaluate(const void* expression, size_t size)
{
	fDataReader.SetTo(expression, size, fAddressSize);

	try {
		return _Evaluate();
	} catch (const EvaluationException& exception) {
		return B_BAD_VALUE;
	} catch (const std::bad_alloc& exception) {
		return B_NO_MEMORY;
	}
}


status_t
DwarfExpressionEvaluator::_Evaluate()
{
	while (fDataReader.BytesRemaining() > 0) {
		uint8 opcode = fDataReader.Read<uint8>(0);

		switch (opcode) {
			case DW_OP_addr:
				_Push(fDataReader.ReadAddress(0));
				break;
			case DW_OP_const1u:
				_Push(fDataReader.Read<uint8>(0));
				break;
			case DW_OP_const1s:
				_Push(fDataReader.Read<int8>(0));
				break;
			case DW_OP_const2u:
				_Push(fDataReader.Read<uint16>(0));
				break;
			case DW_OP_const2s:
				_Push(fDataReader.Read<int16>(0));
				break;
			case DW_OP_const4u:
				_Push(fDataReader.Read<uint32>(0));
				break;
			case DW_OP_const4s:
				_Push(fDataReader.Read<int32>(0));
				break;
			case DW_OP_const8u:
				_Push(fDataReader.Read<uint64>(0));
				break;
			case DW_OP_const8s:
				_Push(fDataReader.Read<int64>(0));
				break;
			case DW_OP_constu:
				_Push(fDataReader.ReadUnsignedLEB128(0));
				break;
			case DW_OP_consts:
				_Push(fDataReader.ReadSignedLEB128(0));
				break;
			case DW_OP_dup:
				_AssertMinStackSize(1);
				_Push(fStack[fStackSize - 1]);
				break;
			case DW_OP_drop:
				_Pop();
				break;
			case DW_OP_over:
				_AssertMinStackSize(1);
				_Push(fStack[fStackSize - 2]);
				break;
			case DW_OP_pick:
			{
				uint8 index = fDataReader.Read<uint8>(0);
				_AssertMinStackSize(index + 1);
				_Push(fStack[fStackSize - index - 1]);
				break;
			}
			case DW_OP_swap:
			{
				_AssertMinStackSize(2);
				std::swap(fStack[fStackSize - 1], fStack[fStackSize - 2]);
				break;
			}
			case DW_OP_rot:
			{
				_AssertMinStackSize(3);
				target_addr_t tmp = fStack[fStackSize - 1];
				fStack[fStackSize - 1] = fStack[fStackSize - 2];
				fStack[fStackSize - 2] = fStack[fStackSize - 3];
				fStack[fStackSize - 3] = tmp;
				break;
			}
			case DW_OP_deref:
// TODO:...
				break;
			case DW_OP_xderef:
// TODO:...
				break;
			case DW_OP_abs:
			case DW_OP_and:
			case DW_OP_div:
			case DW_OP_minus:
			case DW_OP_mod:
			case DW_OP_mul:
			case DW_OP_neg:
			case DW_OP_not:
			case DW_OP_or:
			case DW_OP_plus:
			case DW_OP_plus_uconst:
			case DW_OP_shl:
			case DW_OP_shr:
			case DW_OP_shra:
			case DW_OP_xor:
			case DW_OP_skip:
			case DW_OP_bra:

			case DW_OP_eq:
			case DW_OP_ge:
			case DW_OP_gt:
			case DW_OP_le:
			case DW_OP_lt:
			case DW_OP_ne:

			case DW_OP_regx:
			case DW_OP_fbreg:
			case DW_OP_bregx:
			case DW_OP_piece:
			case DW_OP_deref_size:
			case DW_OP_xderef_size:
			case DW_OP_nop:
			case DW_OP_push_object_address:
			case DW_OP_call2:
			case DW_OP_call4:
			case DW_OP_call_ref:
			case DW_OP_form_tls_address:
			case DW_OP_call_frame_cfa:
			case DW_OP_bit_piece:
				break;

			default:
				if (opcode >= DW_OP_lit0 && opcode <= DW_OP_lit31) {
					_Push(opcode - DW_OP_lit0);
				} else if (opcode >= DW_OP_reg0 && opcode <= DW_OP_reg31) {
// TODO:...
				} else if (opcode >= DW_OP_breg0 && opcode <= DW_OP_breg31) {
// TODO:...
				} else {
					printf("DwarfExpressionEvaluator::_Evaluate(): unsupported "
						"opcode: %u\n", opcode);
					return B_BAD_DATA;
				}
		}
	}

	return fDataReader.HasOverflow() ? B_BAD_DATA : B_OK;
}
