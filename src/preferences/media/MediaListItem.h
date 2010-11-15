// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        MediaListItem.h
//  Author:      Sikosis, Jérôme Duval
//  Description: Media Preferences
//  Created :    June 25, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
#ifndef __MEDIALISTITEM_H__
#define __MEDIALISTITEM_H__


#include <ListItem.h>
#include <MediaAddOn.h>


struct dormant_node_info;

class MediaIcons;
class MediaWindow;

class AudioMixerListItem;
class DeviceListItem;
class NodeListItem;


class MediaListItem : public BListItem {
public:
								MediaListItem();

	virtual	void				AlterWindow(MediaWindow* window) = 0;

	enum media_type {
		AUDIO_TYPE,
		VIDEO_TYPE
	};

	virtual	void				Update(BView* owner, const BFont* font);
	virtual	void				DrawItem(BView* owner, BRect frame,
									bool complete = false);

	// TODO: refactor and remove this:
	virtual	dormant_node_info*	NodeInfo() = 0;
	
	virtual	const char*			Label() = 0;

	
	static	MediaIcons*			Icons() {return sIcons;}
	static	void				SetIcons(MediaIcons* icons) {sIcons = icons;}

	static	int					Compare(const void* itemOne,
									const void* itemTwo);

	// use double dispatch for comparison,
	// returning item->CompareWith(this) * -1
	// -1 : < item; 0 : == item; 1 : > item
	virtual	int					CompareWith(MediaListItem* item) = 0;
	virtual	int					CompareWith(NodeListItem* item) = 0;
	virtual	int					CompareWith(DeviceListItem* item) = 0;
	virtual	int					CompareWith(AudioMixerListItem* item) = 0;

protected:
			struct Renderer;

	virtual void				SetRenderParameters(Renderer& renderer) = 0;

private:

	static	MediaIcons*			sIcons;
};


class NodeListItem : public MediaListItem {
public:
								NodeListItem(dormant_node_info* node,
									media_type type);

			void				SetMediaType(MediaListItem::media_type type);
			void				SetDefaultOutput(bool isDefault);
			bool				IsDefaultOutput() {return fIsDefaultOutput;}
			void				SetDefaultInput(bool isDefault);
			bool				IsDefaultInput() {return fIsDefaultInput;}

	virtual	void				AlterWindow(MediaWindow* window);

	// TODO: refactor and remove this:
	virtual	dormant_node_info*	NodeInfo(){return fNodeInfo;}

	virtual	const char*			Label();
			media_type			Type() {return fMediaType;}

	virtual	int					CompareWith(MediaListItem* item);

	// -1 : < item; 0 : == item; 1 : > item
	virtual	int					CompareWith(NodeListItem* item);
	virtual	int					CompareWith(DeviceListItem* item);
	virtual	int					CompareWith(AudioMixerListItem* item);

private:

	virtual void				SetRenderParameters(Renderer& renderer);

			dormant_node_info*	fNodeInfo;
			bool				fIsAudioMixer;
			media_type			fMediaType;
			bool				fIsDefaultInput;
			bool				fIsDefaultOutput;
};


class DeviceListItem : public MediaListItem {
public:
								DeviceListItem(const char* title,
									MediaListItem::media_type type);

			MediaListItem::media_type Type() {return fMediaType;}

	// TODO: refactor and remove this:
	virtual	dormant_node_info*	NodeInfo(){return NULL;}
	virtual	const char*			Label() {return fTitle;}

	virtual	void				AlterWindow(MediaWindow* window);

	virtual	int					CompareWith(MediaListItem* item);

	// -1 : < item; 0 : == item; 1 : > item
	virtual	int					CompareWith(NodeListItem* item);
	virtual	int					CompareWith(DeviceListItem* item);
	virtual	int					CompareWith(AudioMixerListItem* item);

private:
	virtual void				SetRenderParameters(Renderer& renderer);

			const char*			fTitle;
			media_type			fMediaType;
};


class AudioMixerListItem : public MediaListItem {
public:
								AudioMixerListItem(const char* title);

	// TODO: refactor and remove this:
	virtual	dormant_node_info*	NodeInfo(){return NULL;}

	virtual	const char*			Label() {return fTitle;}

	virtual	void				AlterWindow(MediaWindow* window);

	virtual	int					CompareWith(MediaListItem* item);

	// -1 : < item; 0 : == item; 1 : > item
	virtual	int					CompareWith(NodeListItem* item);
	virtual	int					CompareWith(DeviceListItem* item);
	virtual	int					CompareWith(AudioMixerListItem* item);

private:
	virtual void				SetRenderParameters(Renderer& renderer);

			const char*			fTitle;
};

#endif	/* __MEDIALISTITEM_H__ */
