/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "StringConstants.h"

#include <new>


StringConstants StringConstants::sDefaultInstance;


/*static*/ bool
StringConstants::Init()
{
	new(&sDefaultInstance) StringConstants;
	if (!sDefaultInstance._Init()) {
		sDefaultInstance.Cleanup();
		return false;
	}

	return true;
}


/*static*/ void
StringConstants::Cleanup()
{
	sDefaultInstance.~StringConstants();
}


bool
StringConstants::_Init()
{
	// generate the member variable initializations
	#define DEFINE_STRING_CONSTANT(name, value)	\
		if (!name.SetTo(value))					\
			return false;
	#define DEFINE_STRING_ARRAY_CONSTANT(name, size, ...)	 				\
		{																	\
			const char* const _values[size] = { __VA_ARGS__ };				\
			for (size_t i = 0; i < sizeof(_values) / sizeof(_values[0]);	\
				 i++) {														\
				if (!name[i].SetTo(_values[i]))								\
					return false;											\
			}																\
		}

	#include "StringConstantsPrivate.h"

	#undef DEFINE_STRING_CONSTANT
	#undef DEFINE_STRING_ARRAY_CONSTANT

	return true;
}
