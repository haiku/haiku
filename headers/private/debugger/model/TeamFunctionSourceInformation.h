/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_FUNCTION_SOURCE_INFORMATION_H
#define TEAM_FUNCTION_SOURCE_INFORMATION_H


#include <SupportDefs.h>

class FunctionDebugInfo;
class SourceCode;


class TeamFunctionSourceInformation {
public:
	virtual						~TeamFunctionSourceInformation();

	virtual	status_t			GetActiveSourceCode(FunctionDebugInfo* info,
									SourceCode*& _code) = 0;
									// returns reference
};


#endif	// TEAM_FUNCTION_SOURCE_INFORMATION_H
