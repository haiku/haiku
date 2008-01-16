/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de
 * Distributed under the terms of the MIT License.
 */

#include "debug_variables.h"

#include <string.h>

#include <KernelExport.h>

#include <debug.h>


static const int kVariableCount				= 64;
static const int kTemporaryVariableCount	= 32;
static const char kTemporaryVariablePrefix	= '_';
static const char* const kCommandReturnValueVariable	= "_";

struct Variable {
	char	name[MAX_DEBUG_VARIABLE_NAME_LEN];
	uint64	value;

	inline bool IsUsed() const
	{
		return name[0] != '\0';
	}

	void Init(const char* variableName)
	{
		strlcpy(name, variableName, sizeof(name));
	}

	void Uninit()
	{
		name[0] = '\0';
	}

	inline bool HasName(const char* variableName) const
	{
		return strncmp(name, variableName, sizeof(name)) == 0;
	}
};

static Variable sVariables[kVariableCount];
static Variable sTemporaryVariables[kTemporaryVariableCount];

static bool sTemporaryVariablesObsolete = false;


static inline bool
is_temporary_variable(const char* variableName)
{
	return variableName[0] == kTemporaryVariablePrefix;
}


static Variable*
get_variable(const char* variableName, bool create)
{
	Variable* variables;
	int variableCount;

	// get the variable domain (persistent or temporary)
	if (is_temporary_variable(variableName)) {
		variables = sTemporaryVariables;
		variableCount = kTemporaryVariableCount;
	} else {
		variables = sVariables;
		variableCount = kVariableCount;
	}

	Variable* freeSlot = NULL;

	for (int i = 0; i < variableCount; i++) {
		Variable* variable = variables + i;

		if (!variable->IsUsed()) {
			if (freeSlot == NULL)
				freeSlot = variable;
		} else if (variable->HasName(variableName))
			return variable;
	}

	if (create && freeSlot != NULL) {
		freeSlot->Init(variableName);
		return freeSlot;
	}

	return NULL;
}


// #pragma mark - debugger commands


static int
cmd_unset_variable(int argc, char **argv)
{
	static const char* usage = "usage: unset <variable>\n"
		"Unsets the given variable, if it exists.\n";
	if (argc != 2 || strcmp(argv[1], "--help") == 0) {
		kprintf(usage);
		return 0;
	}

	const char* variable = argv[1];

	if (is_temporary_variable(variable))
		kprintf("You cannot remove temporary variables.\n");
	else if (!remove_debug_variable(variable))
		kprintf("Did not find variable %s.\n", variable);

	return 0;
}


static int
cmd_variables(int argc, char **argv)
{
	static const char* usage = "usage: vars\n"
		"Unsets the given variable, if it exists.\n";
	if (argc != 1) {
		kprintf(usage);
		return 0;
	}

	// persistent variables
	for (int i = 0; i < kVariableCount; i++) {
		Variable& variable = sVariables[i];
		if (variable.IsUsed()) {
			kprintf("%16s: %llu (0x%llx)\n", variable.name, variable.value,
				variable.value);
		}
	}

	// temporary variables
	for (int i = 0; i < kTemporaryVariableCount; i++) {
		Variable& variable = sTemporaryVariables[i];
		if (variable.IsUsed()) {
			kprintf("%16s: %llu (0x%llx)\n", variable.name, variable.value,
				variable.value);
		}
	}

	return 0;
}


// #pragma mark - kernel public functions


bool
is_debug_variable_defined(const char* variableName)
{
	return get_variable(variableName, false) != NULL;
}


bool
set_debug_variable(const char* variableName, uint64 value)
{
	if (sTemporaryVariablesObsolete && is_temporary_variable(variableName)
		&& strcmp(variableName, kCommandReturnValueVariable) != 0) {
		remove_all_temporary_debug_variables();
	}


	if (Variable* variable = get_variable(variableName, true)) {
		variable->value = value;
		return true;
	}

	return false;
}


uint64
get_debug_variable(const char* variableName, uint64 defaultValue)
{
	if (Variable* variable = get_variable(variableName, false))
		return variable->value;

	return defaultValue;
}


bool
remove_debug_variable(const char* variableName)
{
	// We don't allow explicit removal of inidividual temporary variables.
	// This speeds up removing them all.
	if (is_temporary_variable(variableName))
		return false;

	if (Variable* variable = get_variable(variableName, false)) {
		variable->Uninit();
		return true;
	}

	return false;
}


void
remove_all_temporary_debug_variables()
{
	for (int i = 0; i < kTemporaryVariableCount; i++) {
		Variable& variable = sTemporaryVariables[i];
		if (!variable.IsUsed())
			break;

		variable.Uninit();
	}

	// always keep the return value variable defined
	set_debug_variable(kCommandReturnValueVariable, 0);

	sTemporaryVariablesObsolete = false;
}


/*!	Schedules all temporary variables for removal.
	They will be removed, the next time set_debug_variable() is invoked
	for a temporary variable other than the command return value variable.
*/
void
mark_temporary_debug_variables_obsolete()
{
	sTemporaryVariablesObsolete = true;
}


void
debug_variables_init()
{
	// always keep the return value variable defined
	set_debug_variable(kCommandReturnValueVariable, 0);

	add_debugger_command("unset", &cmd_unset_variable,
		"Unsets the given variable");
	add_debugger_command("vars", &cmd_variables,
		"Lists all defined variables with their values");
}
