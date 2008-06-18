/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "Value.h"

#include <stdio.h>

Value::Value(BString* value, Type type)
	: fType(type)
	, fValue(value)
	, fTranslation(NULL)
{
}

Value::~Value()
{
	delete fValue;
	delete fTranslation;
}

void Value::SetType(Type type)
{
	fType = type;
}

Value::Type Value::GetType()
{
	return fType;
}

void Value::SetValue(BString* value)
{
	fValue = value;
}

BString* Value::GetValue()
{
	return fValue;
}

void Value::SetTranslation(BString* translation)
{
	fTranslation = translation;
}

BString* Value::GetTranslation()
{
	return fTranslation;
}

const char* Value::GetValueString()
{
	if (fValue != NULL) {
		return fValue->String();
	}
	return NULL;
}

const char* Value::GetTranslationString()
{
	if (fTranslation != NULL) {
		return fTranslation->String();
	}
	return NULL;
}

const char* Value::ElementForType()
{
	switch (fType) {
		case kSymbolValue: return "Symbol";
			break;
		case kStringValue: return "String";
			break;
		case kInvocationValue: return "Invocation";
			break;
		case kQuotedValue: return "Quoted";
			break;
		case kUnknownValue: return "Unknown";
			break;
	}
	return "NULL";
}

void Value::Print()
{	
	printf("\t\t<%s>\n", ElementForType());
	if (fValue != NULL) {
		printf("\t\t\t<value>%s</value>\n", fValue->String());
	}
	
	if (fTranslation != NULL) {
		printf("\t\t\t<translation>%s</translation>\n", fTranslation->String());
	}
	printf("\t\t</%s>\n", ElementForType());
}
