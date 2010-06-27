/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _TEAPOT_WINDOW_
#define _TEAPOT_WINDOW_


#include <DirectWindow.h>

#include "ObjectView.h"


class TeapotWindow : public BDirectWindow {
	public:
						TeapotWindow(BRect r, const char* name, window_type wt,
							ulong something);
		
		virtual	bool	QuitRequested();
		virtual void	DirectConnected( direct_buffer_info* info );
		virtual void	MessageReceived(BMessage* msg);

	private:
		ObjectView*		fObjectView;
};


#endif
