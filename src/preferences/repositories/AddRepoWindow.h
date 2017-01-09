/*
 * Copyright 2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill
 */
#ifndef ADD_REPO_WINDOW_H
#define ADD_REPO_WINDOW_H


#include <Button.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>


class AddRepoWindow : public BWindow {
public:
							AddRepoWindow(BRect size,
								const BMessenger& messenger);
	virtual void			MessageReceived(BMessage*);
	virtual void			Quit();
	virtual void			FrameResized(float newWidth, float newHeight);

private:
	status_t				_GetClipboardData();
	
	BTextControl*			fText;
	BButton*				fAddButton;
	BButton*				fCancelButton;
	BMessenger				fReplyMessenger;
};


#endif
