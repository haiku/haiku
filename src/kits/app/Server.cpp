/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include <Server.h>

// constructor
BServer::BServer(const char* signature, bool initGUI, status_t *error)
	: BApplication(signature, initGUI, error),
	  fGUIContextInitialized(false)
{
	fGUIContextInitialized = (initGUI && (!error || *error == B_OK));
}

// InitGUIContext
status_t
BServer::InitGUIContext()
{
	if (fGUIContextInitialized)
		return B_OK;

	status_t error = _InitGUIContext();
	fGUIContextInitialized = (error == B_OK);
	return error;
}
