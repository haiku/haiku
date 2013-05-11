/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_CONSTANTS_H
#define STRING_CONSTANTS_H


#include "AutoPackageAttributes.h"
	// for the kAutoPackageAttributeNames array size
#include "String.h"


class StringConstants {
public:
			// generate the member variable declarations
			#define DEFINE_STRING_CONSTANT(name, value) \
				String	name;
			#define DEFINE_STRING_ARRAY_CONSTANT(name, size, ...) \
				String	name[size];

			#include "StringConstantsPrivate.h"

			#undef DEFINE_STRING_CONSTANT
			#undef DEFINE_STRING_ARRAY_CONSTANT

public:
	static	bool				Init();
	static	void				Cleanup();

	static	const StringConstants& Get()
									{ return sDefaultInstance; }

private:
			bool				_Init();

private:
	static	StringConstants		sDefaultInstance;
};


#endif	// STRING_CONSTANTS_H
