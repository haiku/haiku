/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_CONTEXT_H
#define CLI_CONTEXT_H


class Team;
class UserInterfaceListener;


class CliContext {
public:
								CliContext();

			void				Init(Team* team,
									UserInterfaceListener* listener);

			Team*				GetTeam() const	{ return fTeam; }

			void				QuitSession();


private:
			Team*				fTeam;
			UserInterfaceListener* fListener;
};


#endif	// CLI_CONTEXT_H
