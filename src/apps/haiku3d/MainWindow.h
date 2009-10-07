/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */
#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include <DirectWindow.h>

class RenderView;

class MainWindow: public BDirectWindow {
public:
					MainWindow(BRect frame, const char* title);
					~MainWindow();

	virtual bool	QuitRequested();
	virtual void	MessageReceived(BMessage* message);
	virtual void	DirectConnected(direct_buffer_info* info);

protected:
	RenderView*		fRenderView;
};

#endif /* _MAINWINDOW_H */
