// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        MediaViews.h
//  Author:      Sikosis, Jérôme Duval
//  Description: Media Preferences
//  Created :    June 25, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~


#ifndef __MEDIAVIEWS_H__
#define __MEDIAVIEWS_H__
#include <CheckBox.h>
#include <MenuItem.h>
#include <TextView.h>
#include <View.h>

const uint32 ML_RESTART_MEDIA_SERVER = 'resr';
const uint32 ML_SHOW_VOLUME_CONTROL = 'shvc';
const uint32 ML_ENABLE_REAL_TIME = 'enrt';
const uint32 ML_DEFAULT_CHANGE = 'dech';
const uint32 ML_DEFAULTOUTPUT_CHANGE = 'doch';

class BarView : public BView
{
	public:
    	BarView(BRect frame);
    	virtual	void	Draw(BRect updateRect);
    	bool	mDisplay;
};

class SettingsItem : public BMenuItem
{
	public:
		SettingsItem(dormant_node_info *info, BMessage *message, 
			char shortcut = 0, uint32 modifiers = 0);
		dormant_node_info *mInfo;
};

class Settings2Item : public BMenuItem
{
	public:
		Settings2Item(dormant_node_info *info, media_input *input, BMessage *message, 
			char shortcut = 0, uint32 modifiers = 0);
		~Settings2Item();
		dormant_node_info *mInfo;
		media_input *mInput;
};

class SettingsView : public BView
{
	public:
    	SettingsView(BRect frame, bool isVideo);
    	void AddNodes(BList &list, bool isInput);
    	void SetDefault(dormant_node_info &info, bool isInput, int32 outputID = -1);
    	BCheckBox 		*mRealtimeCheckBox;
    	BCheckBox 		*mVolumeCheckBox;
    	BMenu 			*mMenu1;
    	BMenu 			*mMenu2;
    	BMenu			*mMenu3;
    	BTextView		*mRestartTextView;

    private:
    	bool			mIsVideo;
};	

#endif
