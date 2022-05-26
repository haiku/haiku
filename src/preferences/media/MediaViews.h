// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, Haiku
//
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
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
#include <GroupView.h>
#include <MediaAddOn.h>
#include <MenuItem.h>

#include <ObjectList.h>


class BBox;
class BButton;
class BCheckBox;
class BMenu;
class BMenuField;
class BString;
class BStringView;

class MediaWindow;


const uint32 ML_RESTART_MEDIA_SERVER = 'resr';
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


class SettingsView : public BGroupView
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


protected:

			BMenu*				InputMenu() {return fInputMenu;}
			BMenu*				OutputMenu() {return fOutputMenu;}

			BButton*			MakeRestartButton();

			void				_EmptyMenu(BMenu* menu);
			MediaWindow*		_MediaWindow() const;

private:
			void				_PopulateMenu(BMenu* menu, NodeList& nodes,
									const BMessage& message);
			NodeMenuItem*		_FindNodeItem(BMenu* menu,
									const dormant_node_info* nodeInfo);
			void				_ClearMenuSelection(BMenu* menu);

			BMenu* 				fInputMenu;
			BMenu* 				fOutputMenu;
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
