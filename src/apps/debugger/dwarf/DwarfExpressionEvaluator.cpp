/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
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
#include "Tracing.h"
#include "ValueLocation.h"


// number of elements to increase the stack capacity when the stack is full
static const size_t kStackCapacityIncrement = 64;

// maximum number of elements we allow to be pushed on the stack
static const size_t kMaxStackCapacity			= 1024;

// maximum number of operations we allow to be performed for a single expression
// (to avoid running infinite loops forever)
static const uint32 kMaxOperationCount			= 10000;


// #pragma mark - DwarfExpressionEvaluationContext


DwarfExpressionEvaluationContext::DwarfExpressionEvaluationContext(
	const DwarfTargetInterface* targetInterface, uint8 addressSize,
	target_addr_t relocationDelta)
	:
	fTargetInterface(targetInterface),
	fAddressSize(addressSize),
	fRelocationDelta(relocationDelta)
{
}


DwarfExpressionEvaluationContext::~DwarfExpressionEvaluationContext()
{
}


// #pragma mark - EvaluationException


struct DwarfExpressionEvaluator::EvaluationException {
	const char* message;

	EvaluationException(const char* message)
		:
		message(message)
	{
	}
};


// #pragma mark - DwarfExpressionEvaluator


void
DwarfExpressionEvaluator::_AssertMinStackSize(size_t size) const
{
	if (fStackSize < size)
		throw EvaluationException("pop from empty stack");
}


void
DwarfExpressionEvaluator::_Push(target_addr_t value)
{
	// resize the stack, if we hit the capacity
	if (fStackSize == fStackCapacity) {
		if (fStackCapacity >= kMaxStackCapacity)
			throw EvaluationException("stack overflow");

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
DwarfExpressionEvaluator::Push(target_addr_t value)
{
	try {
		_Push(value);
		return B_OK;
	} catch (const EvaluationException& exception) {
		return B_BAD_VALUE;
	} catch (const std::bad_alloc& exception) {
		return B_NO_MEMORY;
	}
}


status_t
DwarfExpressionEvaluator::Evaluate(const void* expression, size_t size,
	target_addr_t& _result)
{
	fDataReader.SetTo(expression, size, fContext->AddressSize());

	try {
		status_t error = _Evaluate(NULL);
		if (error != B_OK)
			return error;
		_result = _Pop();
		return B_OK;
	} catch (const EvaluationException& exception) {
		WARNING("DwarfExpressionEvaluator::Evaluate(): %s\n",
			exception.message);
		return B_BAD_VALUE;
	} catch (const std::bad_alloc& exception) {
		return B_NO_MEMORY;
	}
}


status_t
DwarfExpressionEvaluator::EvaluateLocation(const void* expression, size_t size,
	ValueLocation& _location)
{
	_location.Clear();

	// the empty expression is a valid one
	if (size == 0) {
		ValuePieceLocation piece;
		piece.SetToUnknown();
		piece.SetSize(0);
		return _location.AddPiece(piece) ? B_OK : B_NO_MEMORY;
	}

	fDataReader.SetTo(expression, size, fContext->AddressSize());

	// parse the first (and maybe only) expression
	try {
		// push the object address, if any
		target_addr_t objectAddress;
		if (fContext->GetObjectAddress(objectAddress))
			_Push(objectAddress);

		ValuePieceLocation piece;
		status_t error = _Evaluate(&piece);
		if (error != B_OK)
			return error;

		// if that's all, it's only a simple expression without composition
		if (fDataReader.BytesRemaining() == 0) {
			if (!piece.IsValid())
				piece.SetToMemory(_Pop());
			piece.SetSize(0);
			return _location.AddPiece(piece) ? B_OK : B_NO_MEMORY;
		}

		// there's more, so it must be a composition operator
		uint8 opcode = fDataReader.Read<uint8>(0);
		if (opcode == DW_OP_piece) {
			piece.SetSize(fDataReader.ReadUnsignedLEB128(0));
		} else if (opcode == DW_OP_bit_piece) {
			uint64 bitSize = fDataReader.ReadUnsignedLEB128(0);
			piece.SetSize(bitSize, fDataReader.ReadUnsignedLEB128(0));
		} else
			return B_BAD_DATA;

		// If there's a composition operator, there must be at least two
		// simple expressions, so this must not be the end.
		if (fDataReader.BytesRemaining() == 0)
			return B_BAD_DATA;
	} catch (const EvaluationException& exception) {
		WARNING("DwarfExpressionEvaluator::EvaluateLocation(): %s\n",
			exception.message);
		return B_BAD_VALUE;
	} catch (const std::bad_alloc& exception) {
		return B_NO_MEMORY;
	}

	// parse subsequent expressions (at least one)
	while (fDataReader.BytesRemaining() > 0) {
		// Restrict the data reader to the remaining bytes to prevent jumping
		// back.
		fDataReader.SetTo(fDataReader.Data(), fDataReader.BytesRemaining(),
			fDataReader.AddressSize());

		try {
			// push the object address, if any
			target_addr_t objectAddress;
			if (fContext->GetObjectAddress(objectAddress))
				_Push(objectAddress);

			ValuePieceLocation piece;
			status_t error = _Evaluate(&piece);
			if (error != B_OK)
				return error;

			if (!piece.IsValid())
				piece.SetToMemory(_Pop());

			// each expression must be followed by a composition operator
			if (fDataReader.BytesRemaining() == 0)
				return B_BAD_DATA;

			uint8 opcode = fDataReader.Read<uint8>(0);
			if (opcode == DW_OP_piece) {
				piece.SetSize(fDataReader.ReadUnsignedLEB128(0));
			} else if (opcode == DW_OP_bit_piece) {
				uint64 bitSize = fDataReader.ReadUnsignedLEB128(0);
				piece.SetSize(bitSize, fDataReader.ReadUnsignedLEB128(0));
			} else
				return B_BAD_DATA;
		} catch (const EvaluationException& exception) {
			WARNING("DwarfExpressionEvaluator::EvaluateLocation(): %s\n",
				exception.message);
			return B_BAD_VALUE;
		} catch (const std::bad_alloc& exception) {
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


status_t
DwarfExpressionEvaluator::_Evaluate(ValuePieceLocation* _piece)
{
	TRACE_EXPR_ONLY({
		TRACE_EXPR("DwarfExpressionEvaluator::_Evaluate(%p, %" B_PRIdOFF ")\n",
			fDataReader.Data(), fDataReader.BytesRemaining());
		const uint8* data = (const uint8*)fDataReader.Data();
		int32 count = fDataReader.BytesRemaining();
		for (int32 i = 0; i < count; i++)
			TRACE_EXPR(" %02x", data[i]);
		TRACE_EXPR("\n");
	})

	uint32 operationsExecuted = 0;

	while (fDataReader.BytesRemaining() > 0) {
		uint8 opcode = fDataReader.Read<uint8>(0);

		switch (opcode) {
			case DW_OP_addr:
				TRACE_EXPR("  DW_OP_addr\n");
				_Push(fDataReader.ReadAddress(0) + fContext->RelocationDelta());
				break;
			case DW_OP_const1u:
				TRACE_EXPR("  DW_OP_const1u\n");
				_Push(fDataReader.Read<uint8>(0));
				break;
			case DW_OP_const1s:
				TRACE_EXPR("  DW_OP_const1s\n");
				_Push(fDataReader.Read<int8>(0));
				break;
			case DW_OP_const2u:
				TRACE_EXPR("  DW_OP_const2u\n");
				_Push(fDataReader.Read<uint16>(0));
				break;
			case DW_OP_const2s:
				TRACE_EXPR("  DW_OP_const2s\n");
				_Push(fDataReader.Read<int16>(0));
				break;
			case DW_OP_const4u:
				TRACE_EXPR("  DW_OP_const4u\n");
				_Push(fDataReader.Read<uint32>(0));
				break;
			case DW_OP_const4s:
				TRACE_EXPR("  DW_OP_const4s\n");
				_Push(fDataReader.Read<int32>(0));
				break;
			case DW_OP_const8u:
				TRACE_EXPR("  DW_OP_const8u\n");
				_Push(fDataReader.Read<uint64>(0));
				break;
			case DW_OP_const8s:
				TRACE_EXPR("  DW_OP_const8s\n");
				_Push(fDataReader.Read<int64>(0));
				break;
			case DW_OP_constu:
				TRACE_EXPR("  DW_OP_constu\n");
				_Push(fDataReader.ReadUnsignedLEB128(0));
				break;
			case DW_OP_consts:
				TRACE_EXPR("  DW_OP_consts\n");
				_Push(fDataReader.ReadSignedLEB128(0));
				break;
			case DW_OP_dup:
				TRACE_EXPR("  DW_OP_dup\n");
				_AssertMinStackSize(1);
				_Push(fStack[fStackSize - 1]);
				break;
			case DW_OP_drop:
				TRACE_EXPR("  DW_OP_drop\n");
				_Pop();
				break;
			case DW_OP_over:
				TRACE_EXPR("  DW_OP_over\n");
				_AssertMinStackSize(1);
				_Push(fStack[fStackSize - 2]);
				break;
			case DW_OP_pick:
			{
				TRACE_EXPR("  DW_OP_pick\n");
				uint8 index = fDataReader.Read<uint8>(0);
				_AssertMinStackSize(index + 1);
				_Push(fStack[fStackSize - index - 1]);
				break;
			}
			case DW_OP_swap:
			{
				TRACE_EXPR("  DW_OP_swap\n");
				_AssertMinStackSize(2);
				std::swap(fStack[fStackSize - 1], fStack[fStackSize - 2]);
				break;
			}
			case DW_OP_rot:
			{
				TRACE_EXPR("  DW_OP_rot\n");
				_AssertMinStackSize(3);
				target_addr_t tmp = fStack[fStackSize - 1];
				fStack[fStackSize - 1] = fStack[fStackSize - 2];
				fStack[fStackSize - 2] = fStack[fStackSize - 3];
				fStack[fStackSize - 3] = tmp;
				break;
			}

			case DW_OP_deref:
				TRACE_EXPR("  DW_OP_deref\n");
				_DereferenceAddress(fContext->AddressSize());
				break;
			case DW_OP_deref_size:
				TRACE_EXPR("  DW_OP_deref_size\n");
				_DereferenceAddress(fDataReader.Read<uint8>(0));
				break;
			case DW_OP_xderef:
				TRACE_EXPR("  DW_OP_xderef\n");
				_DereferenceAddressSpaceAddress(fContext->AddressSize());
				break;
			case DW_OP_xderef_size:
				TRACE_EXPR("  DW_OP_xderef_size\n");
				_DereferenceAddressSpaceAddress(fDataReader.Read<uint8>(0));
				break;

			case DW_OP_abs:
			{
				TRACE_EXPR("  DW_OP_abs\n");
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
				TRACE_EXPR("  DW_OP_and\n");
				_Push(_Pop() & _Pop());
				break;
			case DW_OP_div:
			{
				TRACE_EXPR("  DW_OP_div\n");
				int64 top = (int64)_Pop();
				int64 second = (int64)_Pop();
				_Push(top != 0 ? second / top : 0);
				break;
			}
			case DW_OP_minus:
			{
				TRACE_EXPR("  DW_OP_minus\n");
				target_addr_t top = _Pop();
				_Push(_Pop() - top);
				break;
			}
			case DW_OP_mod:
			{
				TRACE_EXPR("  DW_OP_mod\n");
				// While the specs explicitly speak of signed integer division
				// for "div", nothing is mentioned for "mod".
				target_addr_t top = _Pop();
				target_addr_t second = _Pop();
				_Push(top != 0 ? second % top : 0);
				break;
			}
			case DW_OP_mul:
				TRACE_EXPR("  DW_OP_mul\n");
				_Push(_Pop() * _Pop());
				break;
			case DW_OP_neg:
			{
				TRACE_EXPR("  DW_OP_neg\n");
				if (fContext->AddressSize() == 4)
					_Push(-(int32)_Pop());
				else
					_Push(-(int64)_Pop());
				break;
			}
			case DW_OP_not:
				TRACE_EXPR("  DW_OP_not\n");
				_Push(~_Pop());
				break;
			case DW_OP_or:
				TRACE_EXPR("  DW_OP_or\n");
				_Push(_Pop() | _Pop());
				break;
			case DW_OP_plus:
				TRACE_EXPR("  DW_OP_plus\n");
				_Push(_Pop() + _Pop());
				break;
			case DW_OP_plus_uconst:
				TRACE_EXPR("  DW_OP_plus_uconst\n");
				_Push(_Pop() + fDataReader.ReadUnsignedLEB128(0));
				break;
			case DW_OP_shl:
			{
				TRACE_EXPR("  DW_OP_shl\n");
				target_addr_t top = _Pop();
				_Push(_Pop() << top);
				break;
			}
			case DW_OP_shr:
			{
				TRACE_EXPR("  DW_OP_shr\n");
				target_addr_t top = _Pop();
				_Push(_Pop() >> top);
				break;
			}
			case DW_OP_shra:
			{
				TRACE_EXPR("  DW_OP_shra\n");
				target_addr_t top = _Pop();
				int64 second = (int64)_Pop();
				_Push(second >= 0 ? second >> top : -(-second >> top));
					// right shift on negative values is implementation defined
				break;
			}
			case DW_OP_xor:
				TRACE_EXPR("  DW_OP_xor\n");
				_Push(_Pop() ^ _Pop());
				break;

			case DW_OP_bra:
				TRACE_EXPR("  DW_OP_bra\n");
				if (_Pop() == 0)
					break;
				// fall through
			case DW_OP_skip:
			{
				TRACE_EXPR("  DW_OP_skip\n");
				int16 offset = fDataReader.Read<int16>(0);
				if (offset >= 0 ? offset > fDataReader.BytesRemaining()
						: -offset > fDataReader.Offset()) {
					throw EvaluationException("bra/skip: invalid offset");
				}
				fDataReader.SeekAbsolute(fDataReader.Offset() + offset);
				break;
			}

			case DW_OP_eq:
				TRACE_EXPR("  DW_OP_eq\n");
				_Push(_Pop() == _Pop() ? 1 : 0);
				break;
			case DW_OP_ge:
			{
				TRACE_EXPR("  DW_OP_ge\n");
				int64 top = (int64)_Pop();
				_Push((int64)_Pop() >= top ? 1 : 0);
				break;
			}
			case DW_OP_gt:
			{
				TRACE_EXPR("  DW_OP_gt\n");
				int64 top = (int64)_Pop();
				_Push((int64)_Pop() > top ? 1 : 0);
				break;
			}
			case DW_OP_le:
			{
				TRACE_EXPR("  DW_OP_le\n");
				int64 top = (int64)_Pop();
				_Push((int64)_Pop() <= top ? 1 : 0);
				break;
			}
			case DW_OP_lt:
			{
				TRACE_EXPR("  DW_OP_lt\n");
				int64 top = (int64)_Pop();
				_Push((int64)_Pop() < top ? 1 : 0);
				break;
			}
			case DW_OP_ne:
				TRACE_EXPR("  DW_OP_ne\n");
				_Push(_Pop() == _Pop() ? 1 : 0);
				break;

			case DW_OP_push_object_address:
			{
				TRACE_EXPR("  DW_OP_push_object_address\n");
				target_addr_t address;
				if (!fContext->GetObjectAddress(address))
					throw EvaluationException("failed to get object address");
				_Push(address);
				break;
			}
			case DW_OP_call_frame_cfa:
			{
				TRACE_EXPR("  DW_OP_call_frame_cfa\n");
				target_addr_t address;
				if (!fContext->GetFrameAddress(address))
					throw EvaluationException("failed to get frame address");
				_Push(address);
				break;
			}
			case DW_OP_fbreg:
			{
				int64 offset = fDataReader.ReadSignedLEB128(0);
				TRACE_EXPR("  DW_OP_fbreg(%" B_PRId64 ")\n", offset);
				target_addr_t address;
				if (!fContext->GetFrameBaseAddress(address)) {
					throw EvaluationException(
						"failed to get frame base address");
				}
				_Push(address + offset);
				break;
			}
			case DW_OP_form_tls_address:
			{
				TRACE_EXPR("  DW_OP_form_tls_address\n");
				target_addr_t address;
				if (!fContext->GetTLSAddress(_Pop(), address))
					throw EvaluationException("failed to get tls address");
				_Push(address);
				break;
			}

			case DW_OP_regx:
			{
				TRACE_EXPR("  DW_OP_regx\n");
				if (_piece == NULL) {
					throw EvaluationException(
						"DW_OP_regx in non-location expression");
				}
				uint32 reg = fDataReader.ReadUnsignedLEB128(0);
				if (fDataReader.HasOverflow())
					throw EvaluationException("unexpected end of expression");
				_piece->SetToRegister(reg);
				return B_OK;
			}

			case DW_OP_bregx:
			{
				TRACE_EXPR("  DW_OP_bregx\n");
				uint32 reg = fDataReader.ReadUnsignedLEB128(0);
				_PushRegister(reg, fDataReader.ReadSignedLEB128(0));
				break;
			}

			case DW_OP_call2:
				TRACE_EXPR("  DW_OP_call2\n");
				_Call(fDataReader.Read<uint16>(0), true);
				break;
			case DW_OP_call4:
				TRACE_EXPR("  DW_OP_call4\n");
				_Call(fDataReader.Read<uint32>(0), true);
				break;
			case DW_OP_call_ref:
				TRACE_EXPR("  DW_OP_call_ref\n");
				if (fContext->AddressSize() == 4)
					_Call(fDataReader.Read<uint32>(0), false);
				else
					_Call(fDataReader.Read<uint64>(0), false);
				break;

			case DW_OP_piece:
			case DW_OP_bit_piece:
				// are handled in EvaluateLocation()
				if (_piece == NULL)
					return B_BAD_DATA;

				fDataReader.SeekAbsolute(fDataReader.Offset() - 1);
					// put back the operation
				return B_OK;

			case DW_OP_nop:
				TRACE_EXPR("  DW_OP_nop\n");
				break;

			default:
				if (opcode >= DW_OP_lit0 && opcode <= DW_OP_lit31) {
					TRACE_EXPR("  DW_OP_lit%u\n", opcode - DW_OP_lit0);
					_Push(opcode - DW_OP_lit0);
				} else if (opcode >= DW_OP_reg0 && opcode <= DW_OP_reg31) {
					TRACE_EXPR("  DW_OP_reg%u\n", opcode - DW_OP_reg0);
					if (_piece == NULL) {
						// NOTE: Using these opcodes is actually only allowed in
						// location expression, but gcc 2.95.3 does otherwise.
						_PushRegister(opcode - DW_OP_reg0, 0);
					} else {
						_piece->SetToRegister(opcode - DW_OP_reg0);
						return B_OK;
					}
				} else if (opcode >= DW_OP_breg0 && opcode <= DW_OP_breg31) {
					int64 offset = fDataReader.ReadSignedLEB128(0);
					TRACE_EXPR("  DW_OP_breg%u(%" B_PRId64 ")\n",
						opcode - DW_OP_breg0, offset);
					_PushRegister(opcode - DW_OP_breg0, offset);
				} else {
					WARNING("DwarfExpressionEvaluator::_Evaluate(): "
						"unsupported opcode: %u\n", opcode);
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
			throw EvaluationException("invalid dereference size");
	}

	target_addr_t address = _Pop();
	BVariant value;
	if (!fContext->TargetInterface()->ReadValueFromMemory(address, valueType,
			value)) {
		throw EvaluationException("failed to read memory");
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
			throw EvaluationException("invalid dereference size");
	}

	target_addr_t address = _Pop();
	target_addr_t addressSpace = _Pop();
	BVariant value;
	if (!fContext->TargetInterface()->ReadValueFromMemory(addressSpace, address,
			valueType, value)) {
		throw EvaluationException("failed to read memory");
	}

	_Push(value.ToUInt64());
}


void
DwarfExpressionEvaluator::_PushRegister(uint32 reg, target_addr_t offset)
{
	BVariant value;
	if (!fContext->TargetInterface()->GetRegisterValue(reg, value))
		throw EvaluationException("failed to get register");

	_Push(value.ToUInt64() + offset);
}


void
DwarfExpressionEvaluator::_Call(uint64 offset, bool local)
{
	if (fDataReader.HasOverflow())
		throw EvaluationException("unexpected end of expression");

	// get the expression to "call"
	const void* block;
	off_t size;
	if (fContext->GetCallTarget(offset, local, block, size) != B_OK)
		throw EvaluationException("failed to get call target");

	// no expression is OK, then this is just a no-op
	if (block == NULL)
		return;

	// save the current data reader state
	DataReader savedReader = fDataReader;

	// set the reader to the target expression
	fDataReader.SetTo(block, size, savedReader.AddressSize());

	// and evaluate it
	try {
		if (_Evaluate(NULL) != B_OK)
			throw EvaluationException("call failed");
	} catch (...) {
		fDataReader = savedReader;
		throw;
	}

	fDataReader = savedReader;
}
