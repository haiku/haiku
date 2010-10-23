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


#include <Box.h>
#include <ListView.h>
#include <MediaAddOn.h>
#include <ParameterWeb.h>
#include <StringView.h>
#include <Window.h>

#include <ObjectList.h>

#include "MediaViews.h"
#include "MediaListItem.h"
#include "MediaAlert.h"


#define SETTINGS_FILE "MediaPrefs Settings"


class BSeparatorView;


class MediaWindow : public BWindow
{
public:
								MediaWindow(BRect frame);
   								~MediaWindow();
    virtual	bool 				QuitRequested();
    virtual	void				MessageReceived(BMessage* message);
    virtual	void				FrameResized(float width, float height);
    		status_t			InitCheck();

private:

	typedef BObjectList<dormant_node_info> NodeList;


			status_t			InitMedia(bool first);
			void				_FindNodes();
			void				_FindNodes(media_type type, uint64 kind,
									NodeList& into);
			void				_AddNodeItems(NodeList& from, bool isVideo);
			void				_EmptyNodeLists();

			MediaListItem*		FindMediaListItem(dormant_node_info* info);
			void				InitWindow();

	static	status_t			RestartMediaServices(void* data);
	static	bool				UpdateProgress(int stage, const char * message,
									void * cookie);
	
			BBox*				fBox;
			BListView*			fListView;
			BSeparatorView*		fTitleView;
			BView*				fContentView;
			SettingsView*		fAudioView;
			SettingsView*		fVideoView;
    			    
			media_node*			fCurrentNode;
			BParameterWeb*		fParamWeb;
			

			NodeList			fAudioInputs;
			NodeList			fAudioOutputs;
			NodeList			fVideoInputs;
			NodeList			fVideoOutputs;
	
			BList				fIcons;
			MediaAlert*			fAlert;
			status_t			fInitCheck;
};

#endif
