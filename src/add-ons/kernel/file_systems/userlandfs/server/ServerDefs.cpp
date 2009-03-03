/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ServerDefs.h"

// constructor
ServerSettings::ServerSettings()
	: fEnterDebugger(false)
{
}

// destructor
ServerSettings::~ServerSettings()
{
}

// SetEnterDebugger
void
ServerSettings::SetEnterDebugger(bool enterDebugger)
{
	fEnterDebugger = enterDebugger;
}

// ShallEnterDebugger
bool
ServerSettings::ShallEnterDebugger() const
{
	return fEnterDebugger;
}

// the global settings
ServerSettings UserlandFS::gServerSettings;
