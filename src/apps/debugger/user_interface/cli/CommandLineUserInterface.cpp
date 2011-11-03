/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "CommandLineUserInterface.h"


CommandLineUserInterface::CommandLineUserInterface()
{
}


CommandLineUserInterface::~CommandLineUserInterface()
{
}


const char*
CommandLineUserInterface::ID() const
{
	return "BasicCommandLineUserInterface";
}


status_t
CommandLineUserInterface::Init(Team* team, UserInterfaceListener* listener)
{
	return B_UNSUPPORTED;
}


void
CommandLineUserInterface::Show()
{
}


void
CommandLineUserInterface::Terminate()
{
}


status_t
CommandLineUserInterface::LoadSettings(const TeamUISettings* settings)
{
	return B_UNSUPPORTED;
}


status_t
CommandLineUserInterface::SaveSettings(TeamUISettings*& settings) const
{
	return B_UNSUPPORTED;;
}


void
CommandLineUserInterface::NotifyUser(const char* title, const char* message,
	user_notification_type type)
{
}


int32
CommandLineUserInterface::SynchronouslyAskUser(const char* title,
	const char* message, const char* choice1, const char* choice2,
	const char* choice3)
{
	return 0;
}
