/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include <Server.h>

// constructor
BServer::BServer(const char* signature, bool initGUI, status_t* error)
	: BApplication(signature, initGUI, error)
{
}

// InitGUIContext
status_t
BServer::InitGUIContext()
{
	return _InitGUIContext();
}
