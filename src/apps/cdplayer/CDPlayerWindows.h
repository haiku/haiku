/*

CDPlayer Windows Header

Author: Sikosis

(C)2004 Haiku - http://haiku-os.org/

*/

#ifndef __CDPLAYERWINDOWS_H__
#define __CDPLAYERWINDOWS_H__

#include "CDPlayer.h"
#include "CDPlayerViews.h"

class CDPlayerView;

class CDPlayerWindow : public BWindow
{
	public:
		CDPlayerWindow(BRect frame);
		~CDPlayerWindow();
		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage *message);
	private:
		void InitWindow(void);

		void LoadSettings(BMessage *msg);
		void SaveSettings(void);
		CDPlayerView*		ptrCDPlayerView;
};

#endif
