// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, Haiku
//
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
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

struct MediaIcons;
class MediaWindow;

class AudioMixerListItem;
class DeviceListItem;
class MidiListItem;
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

	virtual	const char*			Label() = 0;


	static	MediaIcons*			Icons() {return sIcons;}
	static	void				SetIcons(MediaIcons* icons) {sIcons = icons;}

	struct Visitor {
		virtual	void			Visit(AudioMixerListItem* item) = 0;
		virtual	void			Visit(DeviceListItem* item) = 0;
		virtual	void			Visit(NodeListItem* item) = 0;
		virtual void			Visit(MidiListItem* item) = 0;
	};

	virtual	void				Accept(Visitor& visitor) = 0;

	// use the visitor pattern for comparison,
	// -1 : < item; 0 : == item; 1 : > item
	virtual	int					CompareWith(MediaListItem* item) = 0;

	static	int					Compare(const void* itemOne,
									const void* itemTwo);

protected:
			struct Renderer;

	virtual void				SetRenderParameters(Renderer& renderer) = 0;

private:

	static	MediaIcons*			sIcons;
};


class NodeListItem : public MediaListItem {
public:
								NodeListItem(const dormant_node_info* node,
									media_type type);

			void				SetMediaType(MediaListItem::media_type type);
			void				SetDefaultOutput(bool isDefault);
			bool				IsDefaultOutput() {return fIsDefaultOutput;}
			void				SetDefaultInput(bool isDefault);
			bool				IsDefaultInput() {return fIsDefaultInput;}

	virtual	void				AlterWindow(MediaWindow* window);

	virtual	const char*			Label();
			media_type			Type() {return fMediaType;}

	virtual	void				Accept(MediaListItem::Visitor& visitor);

	struct Comparator : public MediaListItem::Visitor {
								Comparator(NodeListItem* compareOthersTo);
		virtual	void			Visit(NodeListItem* item);
		virtual	void			Visit(DeviceListItem* item);
		virtual	void			Visit(AudioMixerListItem* item);
		virtual void			Visit(MidiListItem* item);

				int				result;
					// -1 : < item; 0 : == item; 1 : > item
	private:
				NodeListItem*	fTarget;
	};

	// -1 : < item; 0 : == item; 1 : > item
	virtual	int					CompareWith(MediaListItem* item);

private:

	virtual void				SetRenderParameters(Renderer& renderer);

			const dormant_node_info* fNodeInfo;

			media_type			fMediaType;
			bool				fIsDefaultInput;
			bool				fIsDefaultOutput;
};


class DeviceListItem : public MediaListItem {
public:
								DeviceListItem(const char* title,
									MediaListItem::media_type type);

			MediaListItem::media_type Type() {return fMediaType;}
	virtual	const char*			Label() {return fTitle;}
	virtual	void				AlterWindow(MediaWindow* window);
	virtual	void				Accept(MediaListItem::Visitor& visitor);

	struct Comparator : public MediaListItem::Visitor {
								Comparator(DeviceListItem* compareOthersTo);
		virtual	void			Visit(NodeListItem* item);
		virtual	void			Visit(DeviceListItem* item);
		virtual	void			Visit(AudioMixerListItem* item);
		virtual void			Visit(MidiListItem* item);

				int				result;
					// -1 : < item; 0 : == item; 1 : > item
	private:
				DeviceListItem*	fTarget;
	};

	// -1 : < item; 0 : == item; 1 : > item
	virtual	int					CompareWith(MediaListItem* item);

private:
	virtual void				SetRenderParameters(Renderer& renderer);

			const char*			fTitle;
			media_type			fMediaType;
};


class AudioMixerListItem : public MediaListItem {
public:
								AudioMixerListItem(const char* title);


	virtual	const char*			Label() {return fTitle;}
	virtual	void				AlterWindow(MediaWindow* window);
	virtual	void				Accept(MediaListItem::Visitor& visitor);

	struct Comparator : public MediaListItem::Visitor {
								Comparator(AudioMixerListItem* compareOthersTo);
		virtual	void			Visit(NodeListItem* item);
		virtual	void			Visit(DeviceListItem* item);
		virtual	void			Visit(AudioMixerListItem* item);
		virtual void			Visit(MidiListItem* item);

				int				result;
					// -1 : < item; 0 : == item; 1 : > item
	private:
				AudioMixerListItem* fTarget;
	};

	// -1 : < item; 0 : == item; 1 : > item
	virtual	int					CompareWith(MediaListItem* item);

private:
	virtual void				SetRenderParameters(Renderer& renderer);

			const char*			fTitle;
};


class MidiListItem : public MediaListItem {
public:
								MidiListItem(const char* title);

	virtual	void				AlterWindow(MediaWindow* window);

	virtual	const char*			Label();

	virtual	void				Accept(MediaListItem::Visitor& visitor);

	struct Comparator : public MediaListItem::Visitor {
								Comparator(MidiListItem* compareOthersTo);
		virtual	void			Visit(NodeListItem* item);
		virtual	void			Visit(DeviceListItem* item);
		virtual	void			Visit(AudioMixerListItem* item);
		virtual void			Visit(MidiListItem* item);

				int				result;
					// -1 : < item; 0 : == item; 1 : > item
	private:
				MidiListItem*	fTarget;
	};

	// -1 : < item; 0 : == item; 1 : > item
	virtual	int					CompareWith(MediaListItem* item);

private:

	virtual void				SetRenderParameters(Renderer& renderer);

			const char*			fTitle;
};
#endif	/* __MEDIALISTITEM_H__ */
