/*

MediaViews Header by Sikosis

(C)2002

*/

#ifndef __MEDIAVIEWS_H__
#define __MEDIAVIEWS_H__

#include "Media.h"
#include "MediaWindows.h"

class MediaView : public BView
{
	public:
    	MediaView(BRect frame);
    	virtual	void	Draw(BRect updateRect);	
};

#endif
