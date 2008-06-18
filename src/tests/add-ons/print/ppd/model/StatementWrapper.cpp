/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "StatementWrapper.h"

static const char* kOpenUIStatement = "OpenUI";
static const char* kCloseUIStatement = "CloseUI";

static const char* kOpenGroupStatement = "OpenGroup";
static const char* kCloseGroupStatement = "CloseGroup";
static const char* kOpenSubGroupStatement = "OpenSubGroup";
static const char* kCloseSubGroupStatement = "CloseSubGroup";

// JCL
static const char* kJCL = "JCL";             
static const char* kJCLOpenUIStatement = "JCLOpenUI";
static const char* kJCLCloseUIStatement = "JCLCloseUI";


StatementWrapper::StatementWrapper(Statement* statement)
	: fStatement(statement)
{
	// nothing to do	
}

GroupStatement::GroupStatement(Statement* statement)
	: StatementWrapper(statement)
{
	// nothing to do	
}

bool GroupStatement::IsUIGroup()
{
	return strcmp(GetKeyword(), kOpenUIStatement) == 0;
}

bool GroupStatement::IsGroup()
{
	return strcmp(GetKeyword(), kOpenGroupStatement) == 0;
}

bool GroupStatement::IsSubGroup()
{
	return strcmp(GetKeyword(), kOpenSubGroupStatement) == 0;
}

bool GroupStatement::IsJCL()
{
	return strstr(GetKeyword(), kJCL) == GetKeyword();
}

bool GroupStatement::IsOpenGroup()
{
	const char* keyword = GetKeyword();
	
	return strcmp(keyword, kOpenUIStatement) == 0 || 
		strcmp(keyword, kOpenGroupStatement) == 0 ||
		strcmp(keyword, kOpenSubGroupStatement) == 0 ||
		strcmp(keyword, kJCLOpenUIStatement) == 0;
}

bool GroupStatement::IsCloseGroup()
{
	const char* keyword = GetKeyword();

	return strcmp(keyword, kCloseUIStatement) == 0 || 
		strcmp(keyword, kCloseGroupStatement) == 0 ||
		strcmp(keyword, kCloseSubGroupStatement) == 0 ||
		strcmp(keyword, kJCLCloseUIStatement) == 0;	
}

Value* GroupStatement::GetValue() 
{
	if (strcmp(GetKeyword(), kOpenUIStatement) == 0 ||
		strcmp(GetKeyword(), kJCLOpenUIStatement) == 0) {
		return GetStatement()->GetOption();
	} else {
		return GetStatement()->GetValue();
	}
}

const char* GroupStatement::GetGroupName()
{
	Value* value = GetValue();
	if (value == NULL) return NULL;
	BString* string = value->GetValue();
	if (string == NULL) return NULL;
	const char* name = string->String();	
	if (name != NULL && *name == '*') {
		// skip '*'
		name ++;
	}
	return name;
}

const char* GroupStatement::GetGroupTranslation()
{
	Value* value = GetValue();
	if (value == NULL) return NULL;
	BString* string = value->GetTranslation();
	if (string == NULL) return NULL;
	return string->String();	
}

GroupStatement::Type GroupStatement::GetType()
{
	const char* type = GetStatement()->GetValueString();
	if (type == NULL) return kUnknown;
	
	if (strstr(type, "PickOne") != NULL) return kPickOne;
	if (strstr(type, "PickMany") != NULL) return kPickMany;
	if (strstr(type, "Boolean") != NULL) return kBoolean;
	
	return kUnknown;
}
