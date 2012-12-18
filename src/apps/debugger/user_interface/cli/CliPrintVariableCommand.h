/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_PRINT_VARIABLE_COMMAND_H
#define CLI_PRINT_VARIABLE_COMMAND_H


#include "CliCommand.h"


class Team;
class StackFrame;
class ValueNode;
class ValueNodeChild;


class CliPrintVariableCommand : public CliCommand {
public:
								CliPrintVariableCommand();
	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context);

private:
			status_t			_ResolveValueIfNeeded(ValueNode* node,
									CliContext& context, int32 maxDepth);
};


#endif	// CLI_PRINT_VARIABLE_COMMAND_H
