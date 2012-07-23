/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef COMMAND_LINE_USER_INTERFACE_H
#define COMMAND_LINE_USER_INTERFACE_H


#include <ObjectList.h>
#include <String.h>

#include "CliContext.h"
#include "UserInterface.h"


class CliCommand;


class CommandLineUserInterface : public UserInterface {
public:
								CommandLineUserInterface();
	virtual						~CommandLineUserInterface();

	virtual	const char*			ID() const;

	virtual	status_t			Init(Team* team,
									UserInterfaceListener* listener);
	virtual	void				Show();
	virtual	void				Terminate();
									// shut down the UI *now* -- no more user
									// feedback

	virtual status_t			LoadSettings(const TeamUISettings* settings);
	virtual status_t			SaveSettings(TeamUISettings*& settings)	const;

	virtual	void				NotifyUser(const char* title,
									const char* message,
									user_notification_type type);
	virtual	int32				SynchronouslyAskUser(const char* title,
									const char* message, const char* choice1,
									const char* choice2, const char* choice3);

			void				Run();
									// Called by the main thread, when
									// everything has been set up. Enters the
									// input loop.

private:
			struct CommandEntry;
			typedef BObjectList<CommandEntry> CommandList;

			struct HelpCommand;

			// GCC 2 support
			friend struct HelpCommand;

private:
	static	status_t			_InputLoopEntry(void* data);
			status_t			_InputLoop();

			status_t			_RegisterCommands();
			bool				_RegisterCommand(const BString& name,
									CliCommand* command);
			void				_ExecuteCommand(int argc,
									const char* const* argv);
			void				_PrintHelp();

private:
			CliContext			fContext;
			CommandList			fCommands;
			sem_id				fShowSemaphore;
			bool				fShown;
			bool				fTerminating;
};


#endif	// COMMAND_LINE_USER_INTERFACE_H
