/*

MediaViews Header by Sikosis

(C)2003

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

class IconView : public BView
{
	public:
    	IconView(BRect frame);
    	virtual	void	Draw(BRect updateRect);	
};

class AvailableViewArea : public BView
{
	public:
    	AvailableViewArea(BRect frame);
    	virtual	void	Draw(BRect updateRect);	
};

class AudioSettingsView : public BView
{
	public:
    	AudioSettingsView(BRect frame);
    	virtual	void	Draw(BRect updateRect);	
};

class AudioMixerView : public BView
{
	public:
    	AudioMixerView(BRect frame);
    	virtual	void	Draw(BRect updateRect);	
};


#endif
