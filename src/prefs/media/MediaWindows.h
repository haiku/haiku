/*

Media Windows Header by Sikosis

(C)2003

*/

#ifndef __MEDIAWINDOWS_H__
#define __MEDIAWINDOWS_H__

#include "Media.h"
#include "MediaViews.h"

class MediaView;

class MediaWindow : public BWindow
{
	public:
    	MediaWindow(BRect frame);
	    ~MediaWindow();
    	virtual bool QuitRequested();
	    virtual void MessageReceived(BMessage *message);
	    virtual void FrameResized(float width, float height); 
	private:
		void InitWindow(void);
		
	    MediaView*	 ptrMediaView;
};

#endif
