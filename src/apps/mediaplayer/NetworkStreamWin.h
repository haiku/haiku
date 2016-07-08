/*
 * Copyright 2016 Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef __NETWORK_STREAM_WIN_H
#define __NETWORK_STREAM_WIN_H


#include <Messenger.h>
#include <TextControl.h>
#include <Window.h>


class NetworkStreamWin : public BWindow
{
public:
								NetworkStreamWin(BMessenger target);
	virtual						~NetworkStreamWin();

	virtual	void				MessageReceived(BMessage* message);

	virtual void				WindowActivated(bool active);
private:
			void				_LookIntoClipboardForUrl();

			BMessenger			fTarget;
			BTextControl*		fTextControl;
};

#endif
