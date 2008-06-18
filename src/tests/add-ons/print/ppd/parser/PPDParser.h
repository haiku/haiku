/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _PPD_PARSER_H
#define _PPD_PARSER_H

#include "PPD.h"
#include "Parser.h"
#include "Statement.h"
#include "StatementWrapper.h"

class RequiredKeywords;

class PPDParser : public Parser
{
private:
	PPD*              fPPD;
	StatementList     fStack; // of nested statements
	RequiredKeywords* fRequiredKeywords;
	bool              fParseAll;
	
	void Push(Statement* statement);
	Statement* Top();
	void Pop();
	
	// Add statement to PPD or the children of a 
	// nested statement
	void AddStatement(Statement* statement);
	
	bool IsValidOpenStatement(GroupStatement* statement);
	bool IsValidCloseStatement(GroupStatement* statement);
	bool ParseStatement(Statement* statement);
	bool ParseStatements();
	PPD* Parse(bool all);
	
public:
	PPDParser(const char* file);
	~PPDParser();
	
	PPD* ParseAll();
	PPD* ParseHeader();
};

#endif
