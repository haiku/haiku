/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "Statement.h"

#include <stdio.h>


Statement::Statement()
	: fType(kUnknown)
	, fKeyword(NULL)
	, fOption(NULL)
	, fValue(NULL)
	, fChildren(NULL)
{
}

Statement::~Statement()
{
	delete fKeyword;
	delete fOption;
	delete fValue;
	delete fChildren;
}

void Statement::SetType(Type type)
{
	fType = type;
}

Statement::Type Statement::GetType()
{
	return fType;
}

void Statement::SetKeyword(BString* keyword)
{
	fKeyword = keyword;
}

BString* Statement::GetKeyword()
{
	return fKeyword;
}

void Statement::SetOption(Value* option)
{
	fOption = option;
}

Value* Statement::GetOption()
{
	return fOption;
}


void Statement::SetValue(Value* value)
{
	fValue = value;
}

Value* Statement::GetValue()
{
	return fValue;
}

StatementList* Statement::GetChildren()
{
	return fChildren;
}

void Statement::AddChild(Statement* statement)
{
	if (fChildren == NULL) {
		fChildren = new StatementList(true);
	}
	fChildren->Add(statement);
}

const char* Statement::GetKeywordString() 
{
	if (fKeyword != NULL) {
		return fKeyword->String();
	}
	return NULL;
}

const char* Statement::GetOptionString()
{
	Value* option = GetOption();
	if (option != NULL) {
		return option->GetValueString();
	}
	return NULL;
}

const char* Statement::GetTranslationString()
{
	Value* option = GetOption();
	if (option != NULL) {
		return option->GetTranslationString();
	}
	return NULL;
}

const char* Statement::GetValueString()
{
	Value* value = GetValue();
	if (value != NULL) {
		return value->GetValueString();
	}
	return NULL;
}

const char* Statement::GetValueTranslationString()
{
	Value* value = GetValue();
	if (value != NULL) {
		return value->GetTranslationString();
	}
	return NULL;
}


const char* Statement::ElementForType() {
	switch (fType) {
		case kDefault: return "Default";
			break;
		case kParam:   return "Param";
			break;
		case kQuery:   return "Query";
			break;
		case kValue:   return "Value";
			break;
		case kUnknown: return "Unknown";
			break;
	}
	return NULL;
}

void Statement::Print()
{
	bool hasValue = fValue != NULL;
	bool hasOption = fOption != NULL;
	
	printf("<%s", ElementForType());
	
	if (fKeyword != NULL) {
		printf(" keyword=\"%s\"", fKeyword->String());
	}
	
	if (hasValue || hasOption) {
		printf(">\n");
	} else {
		printf("/>\n");
	}


	if (hasOption) {
		printf("\t<option>\n");
		fOption->Print();
		printf("\t</option>\n");
	}			
	
	
	if (hasValue) {
		printf("\t<value>\n");
		fValue->Print();
		printf("\t</value>\n");
	}
	
	if (GetChildren() != NULL) {
		printf("\t<children>\n");
		GetChildren()->Print();
		printf("\t</children>\n");
	}

	if (hasValue || hasOption) {
		printf("</%s>\n\n", ElementForType());
	}	
}

