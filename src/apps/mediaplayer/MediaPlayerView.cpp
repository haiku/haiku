#include <Message.h>
#include <Messenger.h>
#include <Rect.h>
#include <Node.h>
#include <stdio.h>

#include "Constants.h"
#include "MediaPlayerView.h"

using namespace BPrivate;

MediaPlayerView::MediaPlayerView(BRect viewFrame, BHandler *handler)
	: BView(viewFrame, "mediaview", B_FOLLOW_ALL, B_FRAME_EVENTS|B_WILL_DRAW)
{ 
	fHandler = handler;
	fMessenger = new BMessenger(handler);
}

MediaPlayerView::~MediaPlayerView()
{
	delete fMessenger;
}
	
void
MediaPlayerView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
}				
