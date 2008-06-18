/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _STATEMENT_WRAPPER_H
#define _STATEMENT_WRAPPER_H

#include "Statement.h"

// wrapper classes to provide specific access to
// statement members

class StatementWrapper
{
private:
	Statement* fStatement;

public:
	StatementWrapper(Statement* statement);

	Statement* GetStatement() { return fStatement; }	
	const char* GetKeyword() { return fStatement->GetKeyword()->String(); }
};

class GroupStatement : public StatementWrapper
{
private:	
	Value* GetValue();
	
public:
	GroupStatement(Statement* statement);

	// test methods if the wrapped statement is a group statement
	bool IsUIGroup();
	bool IsGroup();
	bool IsSubGroup();
	
	bool IsOpenGroup();
	bool IsCloseGroup();

	bool IsJCL();
	
	// accessors	
	const char* GetGroupName();
	const char* GetGroupTranslation();

	enum Type {
		kPickOne,
		kPickMany,
		kBoolean,
		kUnknown
	};
	
	Type GetType();
};

class ConstraintsStatement : public StatementWrapper
{
public:
	ConstraintsStatement(Statement* statement);
	
	// is this realy a constraints statement
	bool IsConstraints();

	const char* GetFirstKeyword();	
	const char* GetFirstOption();	
	const char* GetSecondKeyword();	
	const char* GetSecondOption();	
};

class OrderDependencyStatement : public StatementWrapper
{
public:
	OrderDependencyStatement(Statement* statement);

	// is this realy a order dependency statement	
	bool IsOrderDependency();
	
	// is this a NonUIOrderDependencyStatement
	bool IsNonUI();
	
	float GetOrder();
	const char* GetSection();	
	const char* GetKeyword();
	const char* GetOption();
};

#endif
