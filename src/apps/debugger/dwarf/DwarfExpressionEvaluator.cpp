/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "DwarfExpressionEvaluator.h"

#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <new>

#include <Variant.h>

#include "DataReader.h"
#include "Dwarf.h"
#include "DwarfTargetInterface.h"


// number of elements to increase the stack capacity when the stack is full
static const size_t kStackCapacityIncrement = 64;

// maximum number of elements we allow to be pushed on the stack
static const size_t kMaxStackCapacity			= 1024;

// maximum number of operations we allow to be performed for a single expression
// (to avoid running infinite loops forever)
static const uint32 kMaxOperationCount			= 10000;


// #pragma mark - DwarfExpressionEvaluationContext


DwarfExpressionEvaluationContext::DwarfExpressionEvaluationContext(
	DwarfTargetInterface* targetInterface, uint8 addressSize)
	:
	fTargetInterface(targetInterface),
	fAddressSize(addressSize)
{
}


DwarfExpressionEvaluationContext::~DwarfExpressionEvaluationContext()
{
}


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

		size_t newCapacity = fStackCapacity + kStackCapacityIncrement;
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


DwarfExpressionEvaluator::DwarfExpressionEvaluator(
	DwarfExpressionEvaluationContext* context)
	:
	fContext(context),
	fStack(NULL),
	fStackSize(0),
	fStackCapacity(0)
{
}


DwarfExpressionEvaluator::~DwarfExpressionEvaluator()
{
	free(fStack);
}


status_t
DwarfExpressionEvaluator::Evaluate(const void* expression, size_t size)
{
	fDataReader.SetTo(expression, size, fContext->AddressSize());

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
	uint32 operationsExecuted = 0;

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
				_DereferenceAddress(fContext->AddressSize());
				break;
			case DW_OP_deref_size:
				_DereferenceAddress(fDataReader.Read<uint8>(0));
				break;
			case DW_OP_xderef:
				_DereferenceAddressSpaceAddress(fContext->AddressSize());
				break;
			case DW_OP_xderef_size:
				_DereferenceAddressSpaceAddress(fDataReader.Read<uint8>(0));
				break;

			case DW_OP_abs:
			{
				target_addr_t value = _Pop();
				if (fContext->AddressSize() == 4) {
					int32 signedValue = (int32)value;
					_Push(signedValue >= 0 ? signedValue : -signedValue);
				} else {
					int64 signedValue = (int64)value;
					_Push(signedValue >= 0 ? signedValue : -signedValue);
				}
				break;
			}
			case DW_OP_and:
				_Push(_Pop() & _Pop());
				break;
			case DW_OP_div:
			{
				int64 top = (int64)_Pop();
				int64 second = (int64)_Pop();
				_Push(top != 0 ? second / top : 0);
				break;
			}
			case DW_OP_minus:
			{
				target_addr_t top = _Pop();
				_Push(_Pop() - top);
				break;
			}
			case DW_OP_mod:
			{
				// While the specs explicitly speak of signed integer division
				// for "div", nothing is mentioned for "mod".
				target_addr_t top = _Pop();
				target_addr_t second = _Pop();
				_Push(top != 0 ? second % top : 0);
				break;
			}
			case DW_OP_mul:
				_Push(_Pop() * _Pop());
				break;
			case DW_OP_neg:
			{
				if (fContext->AddressSize() == 4)
					_Push(-(int32)_Pop());
				else
					_Push(-(int64)_Pop());
				break;
			}
			case DW_OP_not:
				_Push(~_Pop());
				break;
			case DW_OP_or:
				_Push(_Pop() | _Pop());
				break;
			case DW_OP_plus:
				_Push(_Pop() + _Pop());
				break;
			case DW_OP_plus_uconst:
				_Push(_Pop() + fDataReader.ReadUnsignedLEB128(0));
				break;
			case DW_OP_shl:
			{
				target_addr_t top = _Pop();
				_Push(_Pop() << top);
				break;
			}
			case DW_OP_shr:
			{
				target_addr_t top = _Pop();
				_Push(_Pop() >> top);
				break;
			}
			case DW_OP_shra:
			{
				target_addr_t top = _Pop();
				int64 second = (int64)_Pop();
				_Push(second >= 0 ? second >> top : -(-second >> top));
					// right shift on negative values is implementation defined
				break;
			}
			case DW_OP_xor:
				_Push(_Pop() ^ _Pop());
				break;

			case DW_OP_bra:
				if (_Pop() == 0)
					break;
				// fall through
			case DW_OP_skip:
			{
				int16 offset = fDataReader.Read<int16>(0);
				if (offset >= 0 ? offset > fDataReader.BytesRemaining()
						: -offset > fDataReader.Offset()) {
					throw EvaluationException();
				}
				fDataReader.SeekAbsolute(fDataReader.Offset() + offset);
				break;
			}

			case DW_OP_eq:
				_Push(_Pop() == _Pop() ? 1 : 0);
				break;
			case DW_OP_ge:
			{
				int64 top = (int64)_Pop();
				_Push((int64)_Pop() >= top ? 1 : 0);
				break;
			}
			case DW_OP_gt:
			{
				int64 top = (int64)_Pop();
				_Push((int64)_Pop() > top ? 1 : 0);
				break;
			}
			case DW_OP_le:
			{
				int64 top = (int64)_Pop();
				_Push((int64)_Pop() <= top ? 1 : 0);
				break;
			}
			case DW_OP_lt:
			{
				int64 top = (int64)_Pop();
				_Push((int64)_Pop() < top ? 1 : 0);
				break;
			}
			case DW_OP_ne:
				_Push(_Pop() == _Pop() ? 1 : 0);
				break;

			case DW_OP_push_object_address:
			{
				target_addr_t address;
				if (!fContext->GetObjectAddress(address))
					throw EvaluationException();
				_Push(address);
				break;
			}
			case DW_OP_call_frame_cfa:
			{
				target_addr_t address;
				if (!fContext->GetFrameAddress(address))
					throw EvaluationException();
				_Push(address);
				break;
			}
			case DW_OP_fbreg:
			{
				target_addr_t address;
				if (!fContext->GetFrameBaseAddress(address))
					throw EvaluationException();
				_Push(address + fDataReader.ReadSignedLEB128(0));
				break;
			}
			case DW_OP_form_tls_address:
			{
				target_addr_t address;
				if (!fContext->GetTLSAddress(_Pop(), address))
					throw EvaluationException();
				_Push(address);
				break;
			}

			case DW_OP_regx:
				_PushRegister(fDataReader.ReadUnsignedLEB128(0), 0);
				break;
			case DW_OP_bregx:
			{
				uint32 reg = fDataReader.ReadUnsignedLEB128(0);
				_PushRegister(reg, fDataReader.ReadSignedLEB128(0));
				break;
			}

			case DW_OP_call2:
				_Call(fDataReader.Read<uint16>(0), true);
				break;
			case DW_OP_call4:
				_Call(fDataReader.Read<uint32>(0), true);
				break;
			case DW_OP_call_ref:
				if (fContext->AddressSize() == 4)
					_Call(fDataReader.Read<uint32>(0), false);
				else
					_Call(fDataReader.Read<uint64>(0), false);
				break;

			case DW_OP_piece:
			case DW_OP_bit_piece:
// TODO:...
				break;

			case DW_OP_nop:
				break;

			default:
				if (opcode >= DW_OP_lit0 && opcode <= DW_OP_lit31) {
					_Push(opcode - DW_OP_lit0);
				} else if (opcode >= DW_OP_reg0 && opcode <= DW_OP_reg31) {
					_PushRegister(opcode - DW_OP_reg0, 0);
				} else if (opcode >= DW_OP_breg0 && opcode <= DW_OP_breg31) {
					_PushRegister(opcode - DW_OP_reg0,
						fDataReader.ReadSignedLEB128(0));
				} else {
					printf("DwarfExpressionEvaluator::_Evaluate(): unsupported "
						"opcode: %u\n", opcode);
					return B_BAD_DATA;
				}
				break;
		}

		if (++operationsExecuted >= kMaxOperationCount)
			return B_BAD_DATA;
	}

	return fDataReader.HasOverflow() ? B_BAD_DATA : B_OK;
}


void
DwarfExpressionEvaluator::_DereferenceAddress(uint8 addressSize)
{
	uint32 valueType;
	switch (addressSize) {
		case 1:
			valueType = B_UINT8_TYPE;
			break;
		case 2:
			valueType = B_UINT16_TYPE;
			break;
		case 4:
			valueType = B_UINT32_TYPE;
			break;
		case 8:
			if (fContext->AddressSize() == 8) {
				valueType = B_UINT64_TYPE;
				break;
			}
			// fall through
		default:
			throw EvaluationException();
	}

	target_addr_t address = _Pop();
	BVariant value;
	if (!fContext->TargetInterface()->ReadValueFromMemory(address, valueType,
			value)) {
		throw EvaluationException();
	}

	_Push(value.ToUInt64());
}


void
DwarfExpressionEvaluator::_DereferenceAddressSpaceAddress(uint8 addressSize)
{
	uint32 valueType;
	switch (addressSize) {
		case 1:
			valueType = B_UINT8_TYPE;
			break;
		case 2:
			valueType = B_UINT16_TYPE;
			break;
		case 4:
			valueType = B_UINT32_TYPE;
			break;
		case 8:
			if (fContext->AddressSize() == 8) {
				valueType = B_UINT64_TYPE;
				break;
			}
			// fall through
		default:
			throw EvaluationException();
	}

	target_addr_t address = _Pop();
	target_addr_t addressSpace = _Pop();
	BVariant value;
	if (!fContext->TargetInterface()->ReadValueFromMemory(addressSpace, address,
			valueType, value)) {
		throw EvaluationException();
	}

	_Push(value.ToUInt64());
}


void
DwarfExpressionEvaluator::_PushRegister(uint32 reg, target_addr_t offset)
{
	BVariant value;
	if (!fContext->TargetInterface()->GetRegisterValue(reg, value))
		throw EvaluationException();

	_Push(value.ToUInt64());
}


void
DwarfExpressionEvaluator::_Call(uint64 offset, bool local)
{
	if (fDataReader.HasOverflow())
		throw EvaluationException();

	// get the expression to "call"
	const void* block;
	off_t size;
	if (fContext->GetCallTarget(offset, local, block, size) != B_OK)
		throw EvaluationException();

	// no expression is OK, then this is just a no-op
	if (block == NULL)
		return;

	// save the current data reader state
	DataReader savedReader = fDataReader;

	// set the reader to the target expression
	fDataReader.SetTo(block, size, savedReader.AddressSize());

	// and evaluate it
	try {
		_Evaluate();
	} catch (...) {
		fDataReader = savedReader;
		throw;
	}

	fDataReader = savedReader;
}
