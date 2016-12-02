/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONNECTION_CONFIG_WINDOW_H
#define CONNECTION_CONFIG_WINDOW_H


#include <Window.h>


class BButton;
class BGroupView;
class BMenu;
class BMenuField;
class TargetHostInterfaceInfo;


class ConnectionConfigWindow : public BWindow
{
public:
								ConnectionConfigWindow();

								~ConnectionConfigWindow();

	static	ConnectionConfigWindow* Create();
									// throws


	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Show();

	virtual	bool				QuitRequested();

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
};

#endif // CONNECTION_CONFIG_WINDOW_H
