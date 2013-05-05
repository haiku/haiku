/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_FSSH_H
#define _FSSH_FSSH_H


#include "compatibility.h"

#include <map>
#include <string>

#include "fssh_defs.h"


typedef fssh_status_t command_function(int argc, const char* const* argv);


namespace FSShell {


enum {
	COMMAND_RESULT_EXIT	= 1,
};


// Command
class Command {
public:
								Command(const char* name,
									const char* description);
								Command(command_function* function,
									const char* name, const char* description);
	virtual						~Command();

			const char*			Name() const;
			const char*			Description() const;

	virtual	fssh_status_t		Do(int argc, const char* const* argv);

private:
			std::string			fName;
			std::string			fDescription;
			command_function*	fFunction;
};


// CommandManager
class CommandManager {
private:
								CommandManager();

public:
	static	CommandManager*		Default();

			void				AddCommand(Command* command);
			void				AddCommand(command_function* function,
									const char* name, const char* description);
			void				AddCommands(command_function* function,
									const char* name, const char* description,
									...);
			Command*			FindCommand(const char* name) const;
			void				ListCommands() const;

private:
	typedef std::map<std::string, Command*> CommandMap;

			CommandMap			fCommands;

	static	CommandManager*		sManager;
};


extern void register_additional_commands(void);


}	// namespace FSShell


#endif	// _FSSH_FSSH_H
