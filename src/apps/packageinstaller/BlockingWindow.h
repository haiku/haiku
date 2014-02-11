/*
 * Copyright (c) 2014, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef BLOCKING_WINDOW_H
#define BLOCKING_WINDOW_H


#include <Window.h>


class BlockingWindow : public BWindow {
public:
								BlockingWindow(BRect frame,
									const char* title, uint32 flags = 0);
	virtual						~BlockingWindow();
		
	virtual	bool				QuitRequested();
		
	virtual	int32				Go();

protected:
			void				ReleaseSem(int32 returnValue);

private:
			sem_id				fSemaphore;
			int32				fReturnValue;
};


#endif // BLOCKING_WINDOW_H

