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

#ifndef ETHERNET_SETTINGS_H
#define ETHERNET_SETTINGS_H

#include <stdlib.h>
#include <unistd.h>
#include <Application.h>

#include "EthernetSettingsWindow.h"

class EthernetSettings : public BApplication {
	public:
		EthernetSettings();
		virtual ~EthernetSettings();
		
		virtual void ReadyToRun();
	private:
		EthernetSettingsWindow	*fEthWindow;
};


#endif /* ETHERNET_SETTINGS_H */
