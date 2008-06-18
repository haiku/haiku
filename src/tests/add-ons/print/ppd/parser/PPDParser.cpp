/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "PPDParser.h"

#include "AutoDelete.h"

#include <stdio.h>
#include <stdlib.h>

// #define VERBOSE 1 

struct Keyword {
	const char* name;
	const char* since;
	int major;
	int minor;
	bool found;
};

static const Keyword gRequiredKeywords[] = {
	{"DefaultImageableArea", NULL},
	{"DefaultPageRegion", NULL},
	{"DefaultPageSize", NULL},
	{"DefaultPaperDimension", NULL},
	// sometimes missing
	// {"FileVersion", NULL},
	// 
	// {"FormatVersion", NULL},
	{"ImageableArea", NULL},
	// "since" is not specified in standard!
	{"LanguageEncoding", "4.3"},
	{"LanguageVersion", NULL},
	{"Manufacturer", "4.3"},
	{"ModelName", NULL},
	{"NickName", NULL},
	{"PageRegion", NULL},
	{"PageSize", NULL},
	{"PaperDimension", NULL},
	{"PCFileName", NULL},
	{"PPD-Adobe", NULL},
	{"Product", NULL},
	{"PSVersion", NULL},
	// sometimes missing
	// {"ShortNickName", "4.3"},
};

// e.g. *PPD.Adobe: "4.3"
const char* kPPDAdobe = "PPD-Adobe";

#define NUMBER_OF_REQUIRED_KEYWORDS (int)(sizeof(gRequiredKeywords) / sizeof(struct Keyword))

class RequiredKeywords
{
private:
	Keyword fKeywords[NUMBER_OF_REQUIRED_KEYWORDS];
	bool    fGotVersion;
	int     fMajorVersion;
	int     fMinorVersion;
	
	void ExtractVersion(int& major, int& minor, const char* version);
	bool IsVersionRequired(Keyword* keyword);

public:
	RequiredKeywords();
	bool IsRequired(Statement* statement);
	bool IsComplete();
	void AppendMissing(BString* string);
};

RequiredKeywords::RequiredKeywords()
	: fGotVersion(false)
{
	for (int i = 0; i < NUMBER_OF_REQUIRED_KEYWORDS; i ++) {
		fKeywords[i] = gRequiredKeywords[i];
		fKeywords[i].found = false;
		const char* since = fKeywords[i].since;
		if (since != NULL) {
			ExtractVersion(fKeywords[i].major, fKeywords[i].minor, since);
		}
	}
}

void RequiredKeywords::ExtractVersion(int& major, int& minor, const char* version)
{
	major = atoi(version);
	minor = 0;
	version = strchr(version, '.');
	if (version != NULL) {
		version ++;
		minor = atoi(version);
	}
}

bool RequiredKeywords::IsVersionRequired(Keyword* keyword)
{
	if (keyword->since == NULL) return true;
	// be conservative if version is missing
	if (!fGotVersion) return true;
	// keyword is not required if file version < since
	if (fMajorVersion < keyword->major) return false;
	if (fMajorVersion == keyword->major &&
		fMinorVersion < keyword->minor) return false;
	return true;
}

bool RequiredKeywords::IsRequired(Statement* statement)
{
	const char* keyword = statement->GetKeyword()->String();

	if (!fGotVersion && strcmp(kPPDAdobe, keyword) == 0 && 
		statement->GetValue() != NULL) {
		Value* value = statement->GetValue();
		BString* string = value->GetValue();
		fGotVersion = true;
		ExtractVersion(fMajorVersion, fMinorVersion, string->String());
	}

	BString defaultKeyword;
	if (statement->GetType() == Statement::kDefault) {
		defaultKeyword << "Default" << keyword;
		keyword = defaultKeyword.String();
	}
	
	for (int i = 0; i < NUMBER_OF_REQUIRED_KEYWORDS; i ++) {
		const char* name = fKeywords[i].name;
		if (strcmp(name, keyword) == 0) {
			fKeywords[i].found = true;
			return true;
		}
	}
	
	return false;
}

bool RequiredKeywords::IsComplete()
{
	for (int i = 0; i < NUMBER_OF_REQUIRED_KEYWORDS; i ++) {
		if (!fKeywords[i].found && IsVersionRequired(&fKeywords[i])) {
			return false;
		}
	}
	return true;
}

void RequiredKeywords::AppendMissing(BString* string)
{
	for (int i = 0; i < NUMBER_OF_REQUIRED_KEYWORDS; i ++) {
		if (!fKeywords[i].found && IsVersionRequired(&fKeywords[i])) {
			*string << "Keyword " << fKeywords[i].name;
			if (fKeywords[i].since != NULL) {
				*string << fKeywords[i].major << ". " << fKeywords[i].minor 
				<< " < " <<
				fMajorVersion << "." << fMinorVersion << " ";
			}
			*string << " is missing\n";
		}
	}	
}

// Constants

static const char* kEndStatement = "End";

// Implementation

PPDParser::PPDParser(const char* file)
	: Parser(file)
	, fStack(false)
	, fRequiredKeywords(new RequiredKeywords)
{
}

PPDParser::~PPDParser()
{
	delete fRequiredKeywords;
}

void PPDParser::Push(Statement* statement)
{
	fStack.Add(statement);
}

Statement* PPDParser::Top()
{
	if (fStack.Size() > 0) {
		return fStack.StatementAt(fStack.Size()-1);
	}
	return NULL;
}

void PPDParser::Pop()
{
	fStack.Remove(Top());
}

void PPDParser::AddStatement(Statement* statement)
{
	fRequiredKeywords->IsRequired(statement);
	
	Statement* top = Top();
	if (top != NULL) {
		top->AddChild(statement);
	} else {
		fPPD->Add(statement);
	}
}

bool PPDParser::IsValidOpenStatement(GroupStatement* statement)
{
	if (statement->GetGroupName() == NULL) {
		Error("Missing group ID in open statement");
		return false;
	}
	return true;
}

bool PPDParser::IsValidCloseStatement(GroupStatement* statement)
{
	if (statement->GetGroupName() == NULL) {
		Error("Missing option in close statement");
		return false;
	}

	if (Top() == NULL) {
		Error("Close statement without an open statement");
		return false;
	}		
	
	GroupStatement openStatement(Top());
		
	// check if corresponding Open* is on top of stack
	BString open = openStatement.GetKeyword();
	open.RemoveFirst("Open");
	BString close = statement->GetKeyword();
	close.RemoveFirst("Close");

	if (open != close) {
		Error("Close statement has no corresponding open statement");
#ifdef VERBOSE
		printf("********* OPEN ************\n");
		openStatement.GetStatement()->Print();
		printf("********* CLOSE ***********\n");
		statement->GetStatement()->Print();
#endif
		return false;
	}

	BString openValue(openStatement.GetGroupName());
	BString closeValue(statement->GetGroupName());

	const char* whiteSpaces = " \t";
	openValue.RemoveSet(whiteSpaces);
	closeValue.RemoveSet(whiteSpaces);

	if (openValue != closeValue) {
		BString message("Open name does not match close name ");
		message << openValue << " != " << closeValue << "\n";
		Warning(message.String());
	}

	return true;
}		

bool PPDParser::ParseStatement(Statement* _statement)
{
	AutoDelete<Statement> statement(_statement);
	
	if (_statement->GetKeyword() == NULL) {
		Error("Keyword missing");
		return false;
	}
	
	if (_statement->GetOption() != NULL && 
		_statement->GetOption()->GetValue() == NULL) {
		// The parser should not provide an option without a value
		Error("Option has no value");
		return false;
	}
	
	if (_statement->GetValue() != NULL && 
		_statement->GetValue()->GetValue() == NULL) {
		// The parser should not provide a value without a value
		Error("Value has no value");
		return false;
	}
	
	const char* keyword = statement.Get()->GetKeyword()->String();
	if (strcmp(keyword, kEndStatement) == 0) {
		// End is ignored
		return true;
	}
	
	GroupStatement group(statement.Get());
	if (group.IsOpenGroup()) {
		
		if (!IsValidOpenStatement(&group)) {
			return false;
		}
		// Add() has to be infront of Push()!
		AddStatement(statement.Release());
		// begin of nested statement
		Push(statement.Get());
		return true;
	}

	if (group.IsCloseGroup()) {

		// end of nested statement
		if (!IsValidCloseStatement(&group)) {
			return false;
		}
	
		Pop();
		// The closing statement is not stored
		return true;
	}
	
	AddStatement(statement.Release());
	return true;
}

bool PPDParser::ParseStatements()
{
	Statement* statement;
	while ((statement = Parser::Parse()) != NULL) {
#ifdef VERBOSE
		statement->Print(); fflush(stdout);
#endif
		if (!ParseStatement(statement)) {
			return false;
		}
		
		if (!fParseAll && fRequiredKeywords->IsComplete()) {
			break;
		}
	}
	
	if (HasError()) {
		return false;
	}
	
	if (Top() != NULL) {
		BString error("Missing close statement for:\n");
		do {
			error << " * " << 
				Top()->GetKeywordString() << " " << 
				Top()->GetOptionString() << "\n";
			Pop();
		} while (Top() != NULL);
		Error(error.String());
		return false;
	}
	return true;
}

PPD* PPDParser::Parse(bool all)
{
	fParseAll = all;
	
	if (InitCheck() != B_OK) return NULL;
	
	fPPD = new PPD();

	ParseStatements();
	
	if (!HasError() && !fRequiredKeywords->IsComplete()) {
		BString string;
		fRequiredKeywords->AppendMissing(&string);
		Error(string.String());
	}
		
	if (HasError()) {
		delete fPPD; fPPD = NULL;
		return NULL;
	}
	
	return fPPD;
}

PPD* PPDParser::ParseAll()
{
	return Parse(true);
}

PPD* PPDParser::ParseHeader()
{
	return Parse(false);
}

