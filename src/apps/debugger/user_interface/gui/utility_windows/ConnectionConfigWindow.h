/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONNECTION_CONFIG_WINDOW_H
#define CONNECTION_CONFIG_WINDOW_H

#include <Window.h>

#include "ConnectionConfigView.h"


class BButton;
class BGroupView;
class BMenu;
class BMenuField;
class Settings;
class TargetHostInterfaceInfo;


class ConnectionConfigWindow : public BWindow,
	private ConnectionConfigView::Listener
{
public:
								ConnectionConfigWindow();

								~ConnectionConfigWindow();

	static	ConnectionConfigWindow* Create();
									// throws


	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Show();

	virtual	bool				QuitRequested();

	// ConnectionConfigView::Listener

	virtual	void				ConfigurationChanged(Settings* settings);

private:
			void	 			_Init();
			BMenu*				_BuildTypeMenu();
			void				_UpdateActiveConfig(
									TargetHostInterfaceInfo* info);

private:
			BMenuField*			fConnectionTypeField;
			BGroupView*			fConfigGroupView;
			BButton*			fCloseButton;
			BButton*			fConnectButton;
			Settings*			fCurrentSettings;
			TargetHostInterfaceInfo*
								fActiveInfo;
};

#endif // CONNECTION_CONFIG_WINDOW_H
