/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _PARSER_H
#define _PARSER_H

#include "Scanner.h"
#include "Statement.h"

// PPD statement parser
class Parser
{
private:
	Scanner fScanner;
	
	int GetCurrentChar() { return fScanner.GetCurrentChar(); }
	void NextChar()      { fScanner.NextChar(); }
	
	void SkipWhitespaces();
	void SkipComment();
	void SkipWhitespaceSeparator();
	bool ParseKeyword(Statement* statement);
	bool ParseTranslation(Value* value, int separator = -1);
	bool ParseOption(Statement* statement);
	bool ParseValue(Statement* statement);
	void UpdateStatementType(Statement* statement);
	Statement* ParseStatement();

protected:
	void Warning(const char* message) { fScanner.Warning(message); }
	void Error(const char* message) { fScanner.Error(message); }
	
public:
	// Initializes the parser with the file
	Parser(const char* file);
	// Returns B_OK if the constructor could open the file
	// successfully
	status_t InitCheck();
	
	// Includes the file for parsing
	bool Include(const char* file) { return fScanner.Include(file); }
	
	// Returns the statement or null on eof or on error
	Statement* Parse();
	// Returns true if there was a parsing error
	bool HasError()               { return fScanner.HasError(); }
	// The error message of the parsing error
	const char* GetErrorMessage() { return fScanner.GetErrorMessage(); }
	// Returns true if there are any warnings
	bool HasWarning()             { return fScanner.HasWarning(); }
	// Returns the waring message
	const char* GetWarningMessage() { return fScanner.GetWarningMessage(); }
};

#endif
