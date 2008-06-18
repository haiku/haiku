/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "AutoDelete.h"
#include "CharacterClasses.h"
#include "Parser.h"

Parser::Parser(const char* file)
	: fScanner(file)
{
	if (InitCheck() == B_OK) {
		NextChar();
	}
}

status_t Parser::InitCheck()
{
	return fScanner.InitCheck();
}

void Parser::SkipWhitespaces()
{
	while (IsWhitespace(GetCurrentChar())) {
		NextChar();
	}
}

void Parser::SkipComment()
{
	while (GetCurrentChar() != kEof && GetCurrentChar() != kCr) {
		NextChar();
	}
	NextChar();
}

void Parser::SkipWhitespaceSeparator()
{
	while (IsWhitespaceSeparator(GetCurrentChar())) {
		NextChar();
	}
}

bool Parser::ParseKeyword(Statement* statement)
{
	// ["?"]
	if (GetCurrentChar() == '?') {
		NextChar();
		statement->SetType(Statement::kQuery);
	}
	// Keyword
	BString* keyword = fScanner.ScanIdent();
	if (keyword == NULL) {
		Error("Identifier expected");
		return false;
	}
	statement->SetKeyword(keyword);
	return true;
}

bool Parser::ParseTranslation(Value* value, int separator)
{
	BString* translation = fScanner.ScanTranslationValue(separator);
	if (translation == NULL) {
		Error("Out of memory scanning translationn!");
		return false;
	}
	value->SetTranslation(translation);
	return true;
}

bool Parser::ParseOption(Statement* statement)
{
	// ["^"]
	bool isSymbolValue = GetCurrentChar() == '^';
	if (isSymbolValue) {
		NextChar();
	}
	
	// [ ... Option ...]
	if (IsOptionChar(GetCurrentChar())) {
		
		BString* option = fScanner.ScanOption();
		if (option == NULL) {
			Error("Out of memory scanning option!");
			return false;
		}
		Value::Type type;
		if (isSymbolValue) {
			type = Value::kSymbolValue;
		} else {
			type = Value::kStringValue;
		}
		Value* value = new Value(option, type);
		statement->SetOption(value);
		
		SkipWhitespaceSeparator();
		// ["/" Translation ]
		if (GetCurrentChar() == '/') {
			NextChar();
			return ParseTranslation(value, ':');
		}
	} else {
		if (isSymbolValue) {
			Error("Expected symbol value!");
			return false;
		}
	}
	return true;
}

bool Parser::ParseValue(Statement* statement)
{
	if (GetCurrentChar() == '"') {
		NextChar();
		
		// "..."
		AutoDelete<Value> value(new Value());
		
		BString* string;
		if (statement->GetOption() != NULL) {
			string = fScanner.ScanInvocationValue();
			value.Get()->SetType(Value::kInvocationValue);
		} else {
			string = fScanner.ScanQuotedValue();
			value.Get()->SetType(Value::kQuotedValue);
		}
		
		if (string == NULL) {
			Error("Expected value");
			return false;
		}
		
		// " is expected
		if (GetCurrentChar() != '"') {
			Error("Expected \" at end of value");
			return false;
		}
		NextChar();
		
		value.Get()->SetValue(string);
		statement->SetValue(value.Release());		
	} else if (GetCurrentChar() == '^') {
		// ^ SymbolValue
		BString* symbol = fScanner.ScanOption();
		if (symbol == NULL) {
			Error("Symbol expected!");
			return false;
		}
		Value* value = new Value(symbol, Value::kSymbolValue);
		statement->SetValue(value);
	} else {
		// StringValue
		BString* stringValue = fScanner.ScanStringValue();
		if (stringValue == NULL) {
			Error("String value expected!");
			return false;
		}
		
		Value* value = new Value(stringValue, Value::kStringValue);
		statement->SetValue(value);
	}
	if (GetCurrentChar() == '/') {
		NextChar();
		return ParseTranslation(statement->GetValue(), kCr);
	}
	return true;
}


void Parser::UpdateStatementType(Statement* statement)
{
	if (statement->GetType() != Statement::kUnknown) return;
	
	BString* keyword = statement->GetKeyword();
	Statement::Type type;
	if (keyword->FindFirst("Default") == 0) {
		type = Statement::kDefault;
		keyword->RemoveFirst("Default");
	} else if (keyword->FindFirst("Param") == 0) {
		type = Statement::kParam;
		keyword->RemoveFirst("Param");
	} else {
		type = Statement::kValue;
	}
	statement->SetType(type);
}

// ["?"]Keyword [["^"]Option["/"Translation]] 
// [":" 
//   ["^"]Value ["/" Translation].
// ]
Statement* Parser::ParseStatement()
{
	AutoDelete<Statement> statement(new Statement());
	
	if (!ParseKeyword(statement.Get())) {
		return NULL;
	}
	SkipWhitespaceSeparator();
	
	if (!ParseOption(statement.Get())) {
		return NULL;
	}
	
	SkipWhitespaceSeparator();
	// [":" ... ]
	if (GetCurrentChar() == ':') {
		NextChar();
		SkipWhitespaceSeparator();
		if (!ParseValue(statement.Get())) {
			return NULL;
		}
	}
	SkipWhitespaceSeparator();
	if (GetCurrentChar() == kEof || GetCurrentChar() == kLf || GetCurrentChar() == kCr) {
		UpdateStatementType(statement.Get());
		Statement* result = statement.Release();	
		return result;
	} else {
		Error("Newline expected at end of statement");
		return NULL;
	}
}

Statement* Parser::Parse()
{
	while (true) {
		int ch = GetCurrentChar();
		if (ch == -1) {
			return NULL;
		}
		if (IsWhitespace(ch)) {
			SkipWhitespaces();
		} else if (ch == '*') {
			// begin of comment or statement
			NextChar();
			ch = GetCurrentChar();
			if (ch == '%') {
				SkipComment();
			} else {
				return ParseStatement();
			}
		} else {
			Error("Expected *");
			return NULL;
		}
	}
}

