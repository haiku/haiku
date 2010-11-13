/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_EXPRESSION_EVALUATOR_H
#define DWARF_EXPRESSION_EVALUATOR_H


#include "DataReader.h"
#include "Types.h"


class DwarfTargetInterface;
class ValueLocation;
struct ValuePieceLocation;


class DwarfExpressionEvaluationContext {
public:
								DwarfExpressionEvaluationContext(
									const DwarfTargetInterface* targetInterface,
									uint8 addressSize,
									target_addr_t relocationDelta);
	virtual						~DwarfExpressionEvaluationContext();

			const DwarfTargetInterface* TargetInterface() const
									{ return fTargetInterface; }
			uint8				AddressSize() const	{ return fAddressSize; }

			target_addr_t			RelocationDelta() const
									{ return fRelocationDelta; }

	virtual	bool				GetObjectAddress(target_addr_t& _address) = 0;
	virtual	bool				GetFrameAddress(target_addr_t& _address) = 0;
	virtual	bool				GetFrameBaseAddress(target_addr_t& _address)
									= 0;
	virtual	bool				GetTLSAddress(target_addr_t localAddress,
									target_addr_t& _address) = 0;

	virtual	status_t			GetCallTarget(uint64 offset, bool local,
									const void*& _block, off_t& _size) = 0;
									// returns error, when an error resolving
									// the entry occurs; returns B_OK and a NULL
									// block, when the entry doesn't have a
									// location attribute

protected:
			const DwarfTargetInterface* fTargetInterface;
			uint8				fAddressSize;
			target_addr_t		fRelocationDelta;
};


class DwarfExpressionEvaluator {
public:
								DwarfExpressionEvaluator(
									DwarfExpressionEvaluationContext* context);
								~DwarfExpressionEvaluator();

			status_t			Push(target_addr_t value);

			status_t			Evaluate(const void* expression, size_t size,
									target_addr_t& _result);
			status_t			EvaluateLocation(const void* expression,
									size_t size, ValueLocation& _location);
									// The returned location will have DWARF
									// semantics regarding register numbers and
									// bit offsets/sizes (cf. bit pieces).

private:
			struct EvaluationException;

private:
	inline	void				_AssertMinStackSize(size_t size) const;

	inline	void				_Push(target_addr_t value);
	inline	target_addr_t		_Pop();

			status_t			_Evaluate(ValuePieceLocation* _piece);
			void				_DereferenceAddress(uint8 addressSize);
			void				_DereferenceAddressSpaceAddress(
									uint8 addressSize);
			void				_PushRegister(uint32 reg, target_addr_t offset);
			void				_Call(uint64 offset, bool local);

private:
			DwarfExpressionEvaluationContext* fContext;
			target_addr_t*		fStack;
			size_t				fStackSize;
			size_t				fStackCapacity;
			DataReader			fDataReader;
			target_addr_t		fObjectAddress;
			target_addr_t		fFrameAddress;
			bool				fObjectAddressValid;
			bool				fFrameAddressValid;
};


#endif	// DWARF_EXPRESSION_EVALUATOR_H
