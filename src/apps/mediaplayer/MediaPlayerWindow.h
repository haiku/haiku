#ifndef MEDIA_PLAYER_WINDOW_H
#define MEDIA_PLAYER_WINDOW_H

#include <FilePanel.h>
#include <MenuBar.h>
#include <Message.h>
#include <Rect.h>
#include <Window.h>

class MediaPlayerView;

class MediaPlayerWindow
	: public BWindow
{
public:
					MediaPlayerWindow(BRect frame);
					MediaPlayerWindow(BRect frame, entry_ref *ref);
	virtual			~MediaPlayerWindow();
	
	virtual void	Quit();
	virtual bool 	QuitRequested();
	virtual void 	MessageReceived(BMessage *message);
	virtual void	MenusBeginning();
	
	void			OpenFile(entry_ref *ref); 

private: 
	void			InitWindow();
	
	BMenuBar		*fMenuBar;
	BMenu			*fOpenFileMenu;
	BMenu			*fOpenURLMenu;
	BMenuItem		*fFileInfoItem;
	BMenuItem		*fCloseItem;
	BMenuItem		*fQuitItem;
	
	BMenuItem		*fMiniModeItem;
	BMenuItem		*fHalfScaleItem;
	BMenuItem		*fNormalScaleItem;
	BMenuItem		*fDoubleScaleItem;
	BMenuItem		*fFullScreenItem;

	BMenuItem		*fApplicationPreferencesItem;
	BMenuItem		*fLoopItem;
	BMenuItem		*fPreserveVideoTimingItem;
	
	MediaPlayerView	*fPlaverView;
};

#endif // MEDIA_PLAYER_WINDOW_H
