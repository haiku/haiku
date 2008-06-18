/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _VALUE_H
#define _VALUE_H

#include <String.h>

class Value {
public:
	enum Type {
		kSymbolValue,
		kStringValue,
		kInvocationValue,
		kQuotedValue,
		kUnknownValue
	};

private:
	Type     fType;
	BString* fValue;
	BString* fTranslation;
	
	const char* ElementForType();
	
public:
	Value(BString* value = NULL, Type type = kUnknownValue);
	virtual ~Value();
	
	void SetType(Type type);
	Type GetType();

	// mandatory in a valid statement	
	void SetValue(BString* value);
	BString* GetValue();

	// optional
	void SetTranslation(BString* translation);
	BString* GetTranslation();

	// convenience methods
	const char* GetValueString();
	const char* GetTranslationString();
	
	void Print();
};

#endif
