/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KEY_REQUEST_WINDOW_H
#define _KEY_REQUEST_WINDOW_H


#include <Message.h>
#include <Window.h>


class KeyRequestView;


class KeyRequestWindow : public BWindow {
public:
									KeyRequestWindow();
virtual								~KeyRequestWindow();

virtual	void						DispatchMessage(BMessage* message,
										BHandler* handler);
virtual	void						MessageReceived(BMessage* message);

		status_t					RequestKey(const BString& keyringName,
										BMessage& keyMessage);

private:
		KeyRequestView*				fRequestView;
		sem_id						fDoneSem;
		status_t					fResult;
};


#endif // _KEY_REQUEST_WINDOW_H
