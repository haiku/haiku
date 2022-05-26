/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de
 * Distributed under the terms of the MIT License.
 */


#include "debug_variables.h"

#include <string.h>

#include <KernelExport.h>

#include <arch/debug.h>
#include <debug.h>
#include <elf.h>
#include <util/DoublyLinkedList.h>


static const int kVariableCount				= 64;
static const int kTemporaryVariableCount	= 32;
static const char kTemporaryVariablePrefix	= '_';
static const char kArchSpecificVariablePrefix = '$';
static const char kSymbolVariablePrefix = '@';
static const char* const kCommandReturnValueVariable = "_";


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

struct TemporaryVariable : Variable,
		DoublyLinkedListLinkImpl<TemporaryVariable> {
	bool queued;
};

static Variable sVariables[kVariableCount];
static TemporaryVariable sTemporaryVariables[kTemporaryVariableCount];

static DoublyLinkedList<TemporaryVariable> sTemporaryVariablesLRUQueue;


static inline bool
is_temporary_variable(const char* variableName)
{
	return variableName[0] == kTemporaryVariablePrefix;
}


static inline bool
is_arch_specific_variable(const char* variableName)
{
	return variableName[0] == kArchSpecificVariablePrefix;
}


static inline bool
is_symbol_variable(const char* variableName)
{
	return variableName[0] == kSymbolVariablePrefix;
}


static void
dequeue_temporary_variable(TemporaryVariable* variable)
{
	// dequeue if queued
	if (variable->queued) {
		sTemporaryVariablesLRUQueue.Remove(variable);
		variable->queued = false;
	}
}


static void
unset_variable(Variable* variable)
{
	if (is_temporary_variable(variable->name))
		dequeue_temporary_variable(static_cast<TemporaryVariable*>(variable));

	variable->Uninit();
}


static void
touch_variable(Variable* _variable)
{
	if (!is_temporary_variable(_variable->name))
		return;

	TemporaryVariable* variable = static_cast<TemporaryVariable*>(_variable);

	// move to the end of the queue
	dequeue_temporary_variable(variable);
	sTemporaryVariablesLRUQueue.Add(variable);
	variable->queued = true;
}


static Variable*
free_temporary_variable_slot()
{
	TemporaryVariable* variable = sTemporaryVariablesLRUQueue.RemoveHead();
	if (variable) {
		variable->queued = false;
		variable->Uninit();
	}

	return variable;
}


static Variable*
get_variable(const char* variableName, bool create)
{
	// find the variable in the respective array and a free slot, we can
	// use, if it doesn't exist yet
	Variable* freeSlot = NULL;

	if (is_temporary_variable(variableName)) {
		// temporary variable
		for (int i = 0; i < kTemporaryVariableCount; i++) {
			TemporaryVariable* variable = sTemporaryVariables + i;

			if (!variable->IsUsed()) {
				if (freeSlot == NULL)
					freeSlot = variable;
			} else if (variable->HasName(variableName))
				return variable;
		}

		if (create && freeSlot == NULL)
			freeSlot = free_temporary_variable_slot();
	} else {
		// persistent variable
		for (int i = 0; i < kVariableCount; i++) {
			Variable* variable = sVariables + i;

			if (!variable->IsUsed()) {
				if (freeSlot == NULL)
					freeSlot = variable;
			} else if (variable->HasName(variableName))
				return variable;
		}
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
	static const char* const usage = "usage: unset <variable>\n"
		"Unsets the given variable, if it exists.\n";
	if (argc != 2 || strcmp(argv[1], "--help") == 0) {
		kprintf(usage);
		return 0;
	}

	const char* variable = argv[1];

	if (!unset_debug_variable(variable))
		kprintf("Did not find variable %s.\n", variable);

	return 0;
}


static int
cmd_unset_all_variables(int argc, char **argv)
{
	static const char* const usage = "usage: %s\n"
		"Unsets all variables.\n";
	if (argc == 2 && strcmp(argv[1], "--help") == 0) {
		kprintf(usage, argv[0]);
		return 0;
	}

	unset_all_debug_variables();

	return 0;
}


static int
cmd_variables(int argc, char **argv)
{
	static const char* const usage = "usage: vars\n"
		"Unsets the given variable, if it exists.\n";
	if (argc != 1) {
		kprintf(usage);
		return 0;
	}

	// persistent variables
	for (int i = 0; i < kVariableCount; i++) {
		Variable& variable = sVariables[i];
		if (variable.IsUsed()) {
			kprintf("%16s: %" B_PRIu64 " (0x%" B_PRIx64 ")\n", variable.name,
				variable.value, variable.value);
		}
	}

	// temporary variables
	for (int i = 0; i < kTemporaryVariableCount; i++) {
		Variable& variable = sTemporaryVariables[i];
		if (variable.IsUsed()) {
			kprintf("%16s: %" B_PRIu64 " (0x%" B_PRIx64 ")\n", variable.name,
				variable.value, variable.value);
		}
	}

	return 0;
}


// #pragma mark - kernel public functions


bool
is_debug_variable_defined(const char* variableName)
{
	if (get_variable(variableName, false) != NULL)
		return true;

	if (is_symbol_variable(variableName))
		return elf_debug_lookup_symbol(variableName + 1) != 0;

	return is_arch_specific_variable(variableName)
		&& arch_is_debug_variable_defined(variableName + 1);
}


bool
set_debug_variable(const char* variableName, uint64 value)
{
	if (is_symbol_variable(variableName))
		return false;

	if (is_arch_specific_variable(variableName))
		return arch_set_debug_variable(variableName + 1, value) == B_OK;

	if (Variable* variable = get_variable(variableName, true)) {
		variable->value = value;
		touch_variable(variable);
		return true;
	}

	return false;
}


uint64
get_debug_variable(const char* variableName, uint64 defaultValue)
{
	if (Variable* variable = get_variable(variableName, false)) {
		touch_variable(variable);
		return variable->value;
	}

	uint64 value;
	if (is_arch_specific_variable(variableName)
		&& arch_get_debug_variable(variableName + 1, &value) == B_OK) {
		return value;
	}

	if (is_symbol_variable(variableName)) {
		addr_t value = elf_debug_lookup_symbol(variableName + 1);
		if (value != 0)
			return value;
	}

	return defaultValue;
}


bool
unset_debug_variable(const char* variableName)
{
	if (Variable* variable = get_variable(variableName, false)) {
		unset_variable(variable);
		return true;
	}

	return false;
}


void
unset_all_debug_variables()
{
	// persistent variables
	for (int i = 0; i < kVariableCount; i++) {
		Variable& variable = sVariables[i];
		if (variable.IsUsed())
			unset_variable(&variable);
	}

	// temporary variables
	for (int i = 0; i < kTemporaryVariableCount; i++) {
		Variable& variable = sTemporaryVariables[i];
		if (variable.IsUsed())
			unset_variable(&variable);
	}
}


void
debug_variables_init()
{
	add_debugger_command("unset", &cmd_unset_variable,
		"Unsets the given variable");
	add_debugger_command("unset_all", &cmd_unset_all_variables,
		"Unsets all variables");
	add_debugger_command("vars", &cmd_variables,
		"Lists all defined variables with their values");
}
