/*
 * Copyright 2005-2015, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef _SERVER_H
#define _SERVER_H


#include <Application.h>


class BServer : public BApplication {
public:
								BServer(const char* signature, bool initGUI,
									status_t *error);
								BServer(const char* signature, const char*
									looperName, port_id port, bool initGUI,
									status_t *error);

			status_t			InitGUIContext();

private:
			bool				fGUIContextInitialized;
};


#endif	// _SERVER_H
