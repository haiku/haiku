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
#include <GridView.h>
#include <MediaAddOn.h>
#include <MenuItem.h>

#include <ObjectList.h>


class BBox;
class BButton;
class BCheckBox;
class BMenu;
class BMenuField;
class BStringView;

class MediaWindow;


const uint32 ML_RESTART_MEDIA_SERVER = 'resr';
const uint32 ML_ENABLE_REAL_TIME = 'enrt';
const uint32 ML_DEFAULT_CHANNEL_CHANGED = 'chch';


class NodeMenuItem : public BMenuItem
{
public:
								NodeMenuItem(const dormant_node_info* info,
									BMessage* message, char shortcut = 0,
									uint32 modifiers = 0);
	virtual	status_t			Invoke(BMessage* message = NULL);

			const dormant_node_info* NodeInfo() const {return fInfo;}
private:

			const dormant_node_info* fInfo;
};


class ChannelMenuItem : public BMenuItem
{
public:
								ChannelMenuItem(media_input* input,
									BMessage* message, char shortcut = 0,
									uint32 modifiers = 0);
	virtual						~ChannelMenuItem();

			int32				DestinationID();
			media_input*		Input();

	virtual	status_t			Invoke(BMessage* message = NULL);

private:
			media_input*		fInput;
};


class SettingsView : public BGridView
{
public:
	typedef BObjectList<dormant_node_info> NodeList;

								SettingsView();
			void				AddInputNodes(NodeList& nodes);
			void				AddOutputNodes(NodeList& nodes);

	virtual	void				SetDefaultInput(const dormant_node_info* info);
	virtual	void				SetDefaultOutput(const dormant_node_info* info);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

			void				RestartRequired(bool required);

protected:

			BMenu*				InputMenu() {return fInputMenu;}
			BMenu*				OutputMenu() {return fOutputMenu;}

			BBox*				MakeRealtimeBox(const char* info,
									uint32 realtimeMask,
									const char* checkboxLabel);
			BStringView*		MakeRestartMessageView();
			BButton*			MakeRestartButton();

			void				_EmptyMenu(BMenu* menu);
			MediaWindow*		_MediaWindow() const;

private:
			void				_FlipRealtimeFlag(uint32 mask);

			void				_PopulateMenu(BMenu* menu, NodeList& nodes,
									const BMessage& message);
			NodeMenuItem*		_FindNodeItem(BMenu* menu,
									const dormant_node_info* nodeInfo);
			void				_ClearMenuSelection(BMenu* menu);

			BCheckBox* 			fRealtimeCheckBox;

			BMenu* 				fInputMenu;
			BMenu* 				fOutputMenu;
			BStringView*		fRestartView;
};	


class AudioSettingsView : public SettingsView
{
public:
								AudioSettingsView();

			void				SetDefaultChannel(int32 channelID);

	virtual	void				SetDefaultInput(const dormant_node_info* info);
	virtual	void				SetDefaultOutput(const dormant_node_info* info);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

private:
			BMenuField*			_MakeChannelMenu();
			BCheckBox*			_MakeVolumeCheckBox();
			void				_FillChannelMenu(const dormant_node_info* info);

			void				_ShowDeskbarVolumeControl();
			void				_HideDeskbarVolumeControl();

			ChannelMenuItem*	_ChannelMenuItemAt(int32 index);

			BMenu*				fChannelMenu;
			BCheckBox* 			fVolumeCheckBox;
};


class VideoSettingsView : public SettingsView
{
public:
								VideoSettingsView();

	virtual	void				SetDefaultInput(const dormant_node_info* info);
	virtual	void				SetDefaultOutput(const dormant_node_info* info);

};

#endif
