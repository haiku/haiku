/*

MediaViews Header by Sikosis

(C)2003

*/

#ifndef __MEDIAVIEWS_H__
#define __MEDIAVIEWS_H__
#include <CheckBox.h>
#include <View.h>
#include <MenuItem.h>

const uint32 ML_RESTART_MEDIA_SERVER = 'resr';
const uint32 ML_SHOW_VOLUME_CONTROL = 'shvc';
const uint32 ML_ENABLE_REAL_TIME = 'enrt';
const uint32 ML_DEFAULT_CHANGE = 'dech';

class BarView : public BView
{
	public:
    	BarView(BRect frame);
    	virtual	void	Draw(BRect updateRect);	
};

class SettingsItem : public BMenuItem
{
	public:
		SettingsItem(dormant_node_info *info, BMessage *message, 
			char shortcut = 0, uint32 modifiers = 0);
		dormant_node_info *mInfo;
};

class SettingsView : public BView
{
	public:
    	SettingsView(BRect frame, bool isVideo);
    	void AddNodes(BList &list, bool isInput);
    	void SetDefault(dormant_node_info &info, bool isInput);
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
