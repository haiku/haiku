/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef REGISTER_H
#define REGISTER_H


#include <SupportDefs.h>


enum register_format {
	REGISTER_FORMAT_INTEGER,
	REGISTER_FORMAT_FLOAT,
	REGISTER_FORMAT_SIMD
};

enum register_type {
	REGISTER_TYPE_INSTRUCTION_POINTER,
	REGISTER_TYPE_STACK_POINTER,
	REGISTER_TYPE_RETURN_ADDRESS,
	REGISTER_TYPE_GENERAL_PURPOSE,
	REGISTER_TYPE_SPECIAL_PURPOSE,
	REGISTER_TYPE_EXTENDED
};


class Register {
public:
								Register(int32 index, const char* name,
									uint32 bitSize, uint32 valueType,
									register_type type, bool calleePreserved);
										// name will not be cloned

			int32				Index() const		{ return fIndex; }
			const char*			Name() const		{ return fName; }
			uint32				ValueType() const	{ return fValueType; }
			register_format		Format() const		{ return fFormat; }
			uint32				BitSize() const		{ return fBitSize; }
			register_type		Type() const		{ return fType; }
			bool				IsCalleePreserved() const
									{ return fCalleePreserved; }

private:
			int32				fIndex;
			const char*			fName;
			uint32				fBitSize;
			uint32				fValueType;
			register_format		fFormat;
			register_type		fType;
			bool				fCalleePreserved;

};


#endif	// REGISTER_H
