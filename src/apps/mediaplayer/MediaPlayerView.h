#ifndef MEDIA_PLAYER_VIEW_H
#define MEDIA_PLAYER_VIEW_H

#include <File.h>
#include <View.h>

class MediaPlayerView : public BView {
public:
	MediaPlayerView(BRect viewframe, BHandler *handler);
	virtual ~MediaPlayerView();

	virtual void FrameResized(float width, float height);
	
private:
	BHandler	*fHandler;
	BMessenger 	*fMessenger;
};

#endif // MEDIA_PLAYER_VIEW_H
