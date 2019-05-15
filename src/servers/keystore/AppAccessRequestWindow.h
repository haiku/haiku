/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _APP_ACCESS_REQUEST_WINDOW_H
#define _APP_ACCESS_REQUEST_WINDOW_H

#include <Bitmap.h>
#include <Button.h>
#include <Message.h>
#include <Window.h>

#include "StripeView.h"

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

virtual	bool						QuitRequested();
virtual	void						MessageReceived(BMessage* message);

		status_t					RequestAppAccess(bool& allowAlways);
		BBitmap						GetIcon(int32 iconSize);
private:
		AppAccessRequestView*		fRequestView;
		sem_id						fDoneSem;
		uint32						fResult;
		BButton* 					fDisallowButton;
		BButton* 					fOnceButton;
		BButton* 					fAlwaysButton;
		BStripeView*				fStripeView;

};


#endif // _APP_ACCESS_REQUEST_WINDOW_H
