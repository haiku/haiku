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


class MediaListItem : public BListItem {
	public:
		MediaListItem(dormant_node_info *info, uint32 level, bool isVideo, BList *icons, uint32 modifiers=0);
		MediaListItem(const char* label, uint32 level, bool isVideo, BList *icons, uint32 modifiers=0);
		~MediaListItem();

		virtual void Update(BView *owner, const BFont *finfo);
		virtual void DrawItem(BView *owner, BRect frame, bool complete = false);

		void SetDefault(bool isDefaultInput, bool isInput);
		void SetAudioMixer(bool isAudioMixer);
		bool IsDefault(bool isInput) { return isInput ? fIsDefaultInput : fIsDefaultOutput; }
		bool IsAudioMixer() { return fIsAudioMixer; }
		bool IsVideo() { return fIsVideo; }
		const char *GetLabel() { return fLabel; }

		dormant_node_info *fInfo;

		static int Compare(const void *firstArg, const void *secondArg);

	private:
		const char *fLabel;
		bool fIsAudioMixer;
		bool fIsVideo;
		bool fIsDefaultInput;
		bool fIsDefaultOutput;
		//dormant_node_info fNodeInfo;
		BList *fIcons;
};

#endif	/* __MEDIALISTITEM_H__ */
