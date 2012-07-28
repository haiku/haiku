/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_CONTEXT_H
#define CLI_CONTEXT_H


#include <sys/cdefs.h>
	// Needed in histedit.h.
#include <histedit.h>

#include <Locker.h>


class Team;
class UserInterfaceListener;


class CliContext {
public:
								CliContext();
								~CliContext();

			status_t			Init(Team* team,
									UserInterfaceListener* listener);
			void				Cleanup();

			void				Terminating();

			// service methods for the input loop thread follow

			Team*				GetTeam() const	{ return fTeam; }

			const char*			PromptUser(const char* prompt);
			void				AddLineToInputHistory(const char* line);

			void				QuitSession(bool killTeam);

			void				WaitForThreadOrUser();

private:
	static	const char*			_GetPrompt(EditLine* editLine);

private:
			BLocker				fLock;
			Team*				fTeam;
			UserInterfaceListener* fListener;
			EditLine*			fEditLine;
			History*			fHistory;
			const char*			fPrompt;
			sem_id				fBlockingSemaphore;
			bool				fInputLoopWaiting;
	volatile bool				fTerminating;
};


#endif	// CLI_CONTEXT_H
