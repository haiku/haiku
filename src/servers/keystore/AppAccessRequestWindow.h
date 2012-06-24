/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _APP_ACCESS_REQUEST_WINDOW_H
#define _APP_ACCESS_REQUEST_WINDOW_H


#include <Message.h>
#include <Window.h>


class AppAccessRequestView;


class AppAccessRequestWindow : public BWindow {
public:
									AppAccessRequestWindow(
										const char* keyringName,
										const char* signature,
										const char* path,
										const char* accessString, bool appIsNew,
										bool appWasUpdated);
virtual								~AppAccessRequestWindow();

virtual	void						DispatchMessage(BMessage* message,
										BHandler* handler);
virtual	void						MessageReceived(BMessage* message);

		status_t					RequestAppAccess(bool& allowAlways);

private:
		AppAccessRequestView*		fRequestView;
		sem_id						fDoneSem;
		uint32						fResult;
};


#endif // _APP_ACCESS_REQUEST_WINDOW_H
