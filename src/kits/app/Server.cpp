/*
 * Copyright 2005-2015, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */


#include <Server.h>


BServer::BServer(const char* signature, bool initGUI, status_t *error)
	:
	BApplication(signature, NULL, -1, initGUI, error),
	fGUIContextInitialized(false)
{
	fGUIContextInitialized = initGUI && (error == NULL || *error == B_OK);
}


BServer::BServer(const char* signature, const char* looperName, port_id port,
	bool initGUI, status_t *error)
	:
	BApplication(signature, looperName, port, initGUI, error),
	fGUIContextInitialized(false)
{
	fGUIContextInitialized = initGUI && (error == NULL || *error == B_OK);
}


status_t
BServer::InitGUIContext()
{
	if (fGUIContextInitialized)
		return B_OK;

	status_t error = _InitGUIContext();
	fGUIContextInitialized = (error == B_OK);
	return error;
}
