/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef COMMAND_H
#define COMMAND_H


#include <ObjectList.h>
#include <String.h>

#include "CommonOptions.h"


class Command {
public:
								Command(const BString& name,
									const BString& shortUsage,
									const BString& longUsage,
									const BString& category);
	virtual						~Command();

			void				Init(const char* programName);

			const BString&		Name() const		{ return fName; }
			const BString&		ShortUsage() const	{ return fShortUsage; }
			const BString&		LongUsage() const	{ return fLongUsage; }
			const BString&		Category() const	{ return fCategory; }

			void				PrintUsage(bool error) const;
			void				PrintUsageAndExit(bool error) const;

	virtual	int					Execute(int argc, const char* const* argv) = 0;

protected:
			CommonOptions		fCommonOptions;

private:
			BString				fName;
			BString				fShortUsage;
			BString				fLongUsage;
			BString				fCategory;
};


typedef BObjectList<Command> CommandList;


class CommandManager {
public:
	static	CommandManager*		Default();

			void				RegisterCommand(Command* command);
			void				InitCommands(const char* programName);

			const CommandList&	Commands() const
									{ return fCommands; }
			void				GetCommands(const char* prefix,
									CommandList& _commands);
			void				GetCommandsForCategory(const char* category,
									CommandList& _commands);

private:
								CommandManager();

private:
			CommandList			fCommands;
};


template<typename CommandType>
struct CommandRegistrar {
	CommandRegistrar()
	{
		CommandManager::Default()->RegisterCommand(new CommandType);
	}
};


#define DEFINE_COMMAND(className, name, shortUsage, longUsage, category)	\
	struct className : Command {								\
		className()												\
			:													\
			Command(name, shortUsage, longUsage, category)		\
		{														\
		}														\
																\
		virtual int Execute(int argc, const char* const* argv);	\
	};															\
	static CommandRegistrar<className> sRegister##className;


#endif	// COMMAND_H
