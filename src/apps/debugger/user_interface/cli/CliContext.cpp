/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CliContext.h"

#include "UserInterface.h"


CliContext::CliContext()
	:
	fTeam(NULL),
	fListener(NULL)
{
}


void
CliContext::Init(Team* team, UserInterfaceListener* listener)
{
	fTeam = team;
	fListener = listener;
}


void
CliContext::QuitSession()
{
	fListener->UserInterfaceQuitRequested();
}
