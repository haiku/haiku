/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _STATEMENT_H
#define _STATEMENT_H

#include "StatementList.h"
#include "Value.h"

class Statement
{
public:
	enum Type {
		kDefault,
		kQuery,
		kValue,
		kParam,
		kUnknown
	};

private:
	Type           fType;
	BString*       fKeyword;
	Value*         fOption;
	Value*         fValue;
	StatementList* fChildren;

	const char* ElementForType();
	
public:
	Statement();
	virtual ~Statement();
	
	void SetType(Type type);	
	Type GetType();
	
	void SetKeyword(BString* keyword);
	// mandatory in a valid statement
	BString* GetKeyword();

	void SetOption(Value* value);	
	// optional in a valid statement
	Value* GetOption();

	void SetValue(Value* value);
	// optional in a valid statement
	Value* GetValue(); 

	void AddChild(Statement* statement);
	// optional in a valid statement
	StatementList* GetChildren();
	
	// convenience methods
	bool IsDefaultStatement() { return fType == kDefault; }
	bool IsQueryStatement()   { return fType == kQuery; }
	bool IsValueStatement()   { return fType == kValue; }
	bool IsParamStatement()   { return fType == kParam; }
	bool IsUnknownStatement() { return fType == kUnknown; }
	
	const char* GetKeywordString();
	const char* GetOptionString();
	const char* GetTranslationString();
	const char* GetValueString();
	const char* GetValueTranslationString();
	
	void Print();
};

#endif
