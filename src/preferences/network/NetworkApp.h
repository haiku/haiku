/*
 * Copyright 2004-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Andre Alves Garzia, andre@andregarzia.com
 * With code from:
 *		Axel Dorfler
 *		Hugo Santos
 */

#ifndef NETWORK_APP_H
#define NETWORK_APP_H

#include <stdlib.h>
#include <unistd.h>
#include <Application.h>
#include <Catalog.h>
#include <Locale.h>

#include "NetworkWindow.h"

class NetworkApp : public BApplication {
	public:
		NetworkApp();
		virtual ~NetworkApp();
		
		virtual void ReadyToRun();
	private:
		NetworkWindow	*fEthWindow;
};


#endif /* NETWORK_APP_H */
