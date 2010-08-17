/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef WINDOW_STACK_H
#define WINDOW_STACK_H


#include <Window.h>


class BWindowStack {
public:
								BWindowStack(BWindow* window);
								~BWindowStack();

		status_t				InitCheck();

		status_t				AddWindow(BWindow* window);
		status_t				AddWindow(BMessenger& window);
		status_t				AddWindowAt(BWindow* window, int32 position);
		status_t				AddWindowAt(BMessenger& window,
									int32 position);

		status_t				RemoveWindow(BWindow* window);
		status_t				RemoveWindow(BMessenger& window);
		status_t				RemoveWindowAt(int32 position,
									BMessenger* window = NULL);

		int32					CountWindows();

		status_t				WindowAt(int32 position,
									BMessenger& messenger);
		bool					HasWindow(BWindow* window);
		bool					HasWindow(BMessenger& window);

private:
		status_t				_AttachMessenger(BMessenger& window);
		status_t				_ReadMessenger(BMessenger& window);
		status_t				_StartMessage(int32 what);

		BPrivate::PortLink*		fLink;
};


#endif
