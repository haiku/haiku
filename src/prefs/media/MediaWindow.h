// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        MediaWindow.h
//  Author:      Sikosis, Jérôme Duval
//  Description: Media Preferences
//  Created :    June 25, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef __MEDIAWINDOWS_H__
#define __MEDIAWINDOWS_H__

#include <MediaAddOn.h>
#include <Window.h>
#include <ParameterWeb.h>
#include <StringView.h>
#include <ListView.h>
#include <Box.h>

#include "MediaViews.h"
#include "MediaListItem.h"
#include "MediaAlert.h"

#define SETTINGS_FILE "MediaPrefs Settings"

class MediaWindow : public BWindow
{
	public:
    	MediaWindow(BRect frame);
	    ~MediaWindow();
	    virtual bool QuitRequested();
	    virtual void MessageReceived(BMessage *message);
	    virtual void FrameResized(float width, float height);
	    status_t InitCheck();
	private:
		status_t InitMedia(bool first);
		void FindNodes(media_type type, uint64 kind, BList &list);
		void AddNodes(BList &list, bool isVideo);
		MediaListItem *FindMediaListItem(dormant_node_info *info);
		void InitWindow(void);
		static status_t RestartMediaServices(void *data);
		static bool UpdateProgress(int stage, const char * message, void * cookie);
		
		BBox *					mBox;
		BListView*				mListView;
		BStringView*			mTitleView;
		BView*					mContentView;
		SettingsView*			mAudioView;
		SettingsView*			mVideoView;
		BarView*				mBar;
	    	    
	    media_node*				mCurrentNode;
	    BParameterWeb*			mParamWeb;
		
		BList					mAudioInputs;
		BList					mAudioOutputs;
		BList					mVideoInputs;
		BList					mVideoOutputs;
		
		BList					mIcons;
		MediaAlert				*mAlert;
		status_t				mInitCheck;
};

#endif
