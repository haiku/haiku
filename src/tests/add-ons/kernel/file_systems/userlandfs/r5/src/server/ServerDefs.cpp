// ServerDefs.cpp

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
ServerSettings gServerSettings;

const char* kUserlandFSDispatcherClipboardName = "userland fs dispatcher";

