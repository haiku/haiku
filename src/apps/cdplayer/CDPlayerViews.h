/*

CDPlayer Views Header

Author: Sikosis

(C)2004 Haiku - http://haiku-os.org/

*/

#ifndef __CDPlayerVIEWS_H__
#define __CDPlayerVIEWS_H__

#include "CDPlayer.h"
#include "CDPlayerWindows.h"

class CDPlayerView : public BView
{
	public:
		CDPlayerView(BRect frame);
		virtual void Draw(BRect updateRect);
};

#endif
