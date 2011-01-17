/*
 * Copyright 2004-2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Andre Alves Garzia, andre@andregarzia.com
 * 		Fredrik Modéen
 */
#ifndef NETWORK_WINDOW_H
#define NETWORK_WINDOW_H


#include <Window.h>

class Settings;
class EthernetSettingsView;


class NetworkWindow : public BWindow {
public:
					NetworkWindow(Settings* setting);
	virtual			~NetworkWindow();
	virtual bool	QuitRequested();
	virtual void	MessageReceived(BMessage* mesage);

private:
	EthernetSettingsView* fEthernetView;
};

#endif	/* NETWORK_WINDOW_H */
