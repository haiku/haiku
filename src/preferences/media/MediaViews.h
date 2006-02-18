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
#include <StringView.h>
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
    	bool	fDisplay;
};

class SettingsItem : public BMenuItem
{
	public:
		SettingsItem(dormant_node_info *info, BMessage *message, 
			char shortcut = 0, uint32 modifiers = 0);
		dormant_node_info *fInfo;
};

class Settings2Item : public BMenuItem
{
	public:
		Settings2Item(dormant_node_info *info, media_input *input, BMessage *message, 
			char shortcut = 0, uint32 modifiers = 0);
		~Settings2Item();
		dormant_node_info *fInfo;
		media_input *fInput;
};

class SettingsView : public BView
{
	public:
    	SettingsView(BRect frame, bool isVideo);
    	void AddNodes(BList &list, bool isInput);
    	void SetDefault(dormant_node_info &info, bool isInput, int32 outputID = -1);
    	BCheckBox 		*fRealtimeCheckBox;
    	BCheckBox 		*fVolumeCheckBox;
    	BMenu 			*fMenu1;
    	BMenu 			*fMenu2;
    	BMenu			*fMenu3;
    	BStringView		*fRestartView;

    private:
    	bool			fIsVideo;
};	

#endif
