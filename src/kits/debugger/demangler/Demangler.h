/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEMANGLER_H
#define DEMANGLER_H

#include <String.h>


class Demangler {
public:
	static	BString				Demangle(const BString& mangledName);
};


#endif	// DEMANGLER_H
