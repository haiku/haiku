/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "Parser.h"
#include "Test.h"

#include <StopWatch.h>
#include <stdio.h>

void TestParser()
{
	Parser parser(gPPDFile);
	if (parser.InitCheck() != B_OK) {
		fprintf(stderr, "Could not open ppd file %s\n", gPPDFile);
		return;
	}
	
	Statement* statement;
	do {
		statement = parser.Parse();
		if (statement != NULL) {
			statement->Print();
		}
		delete statement;
	} while (statement != NULL);
}

#include "PPDParser.h"

static PPD* OpenTestFile(bool all, bool timing)
{
	BStopWatch* stopWatch = NULL; 
	if (timing) {
		stopWatch = new BStopWatch("PPDParser");
	}
	PPDParser parser(gPPDFile);

	if (parser.InitCheck() != B_OK) {
		fprintf(stderr, "Could not open ppd file %s\n", gPPDFile);
		return NULL;
	}
	
	PPD* ppd = all ? parser.ParseAll() : parser.ParseHeader();

	delete stopWatch;
	
	if (ppd == NULL) {
		fprintf(stderr, "Parser returned NULL\n");
		fprintf(stderr, "%s\n", parser.GetErrorMessage());
		return NULL;
	}
	
	return ppd;
}

void TestPPDParser(bool all, bool verbose)
{
	PPD* ppd = OpenTestFile(all, !verbose);
	if (ppd == NULL) return;	
	if (verbose) {
		ppd->Print();
	}
	delete ppd;
}

void ExtractChildren(StatementList* list, int level);

void Indent(int level)
{
	for (; level > 0; level --) {
		printf("  ");
	} 
}

void PrintValue(const char* label, Value* arg, int level)
{	
	Indent(level);
	
	if (label != NULL) {
		printf("%s ", label);
	}
	
	if (arg != NULL) {
		BString* value = arg->GetValue();
		BString* translation = arg->GetTranslation();
		if (translation != NULL) {
			printf("%s", translation->String());
		}
		if (value != NULL) {
			printf(" [%s]", value->String());
		}
	} else {
		printf("NULL");
	}	
	
	printf("\n");
}

bool ExtractGroup(Statement* statement, int level)
{
	GroupStatement group(statement);
	if (group.IsOpenGroup()) {
		const char* translation = group.GetGroupTranslation();
		Indent(level);
		if (translation != NULL) {
			printf("%s", translation);
		}
		const char* name = group.GetGroupName();
		if (name != NULL) {
			printf("[%s]", name);
		}
		printf("\n");
		ExtractChildren(statement->GetChildren(), level+1);
		return true;
	}
	return false;
}

void ExtractChildren(StatementList* list, int level)
{
	if (list == NULL) return;
	for (int32 i = 0; i < list->Size(); i ++) {
		Statement* statement = list->StatementAt(i);
		if (!ExtractGroup(statement, level)) {
			if (statement->GetType() == Statement::kValue) {
				PrintValue(NULL, statement->GetOption(), level);
			} else if (statement->GetType() == Statement::kDefault) {
				PrintValue("Default", statement->GetValue(), level);			
			}
		}
	}	
}

void TestExtractUI()
{
	PPD* ppd = OpenTestFile(true, false);
	if (ppd == NULL) return;
	
	for (int32 i = 0; i < ppd->Size(); i++) {
		Statement* statement = ppd->StatementAt(i);
		ExtractGroup(statement, 0);
	}
}
