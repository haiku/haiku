// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        MediaListItem.cpp
//  Author:      Sikosis, Jérôme Duval
//  Description: Media Preferences
//  Created :    June 25, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~


// Includes -------------------------------------------------------------------------------------------------- //
#include <View.h>

#include "MediaListItem.h"

#define kITEM_MARGIN					  1

// MediaListItem - Constructor
MediaListItem::MediaListItem(dormant_node_info *info, uint32 level, bool isVideo, BList *icons, uint32 modifiers=0) 
	: BListItem(level),
	mIsAudioMixer(false),
	mIsVideo(isVideo),
	mIsDefaultInput(false),
	mIsDefaultOutput(false)
{
	mIcons = icons;
	mInfo = info;
	mLabel = mInfo->name;
	
	SetHeight(16 + kITEM_MARGIN);
}

MediaListItem::MediaListItem(const char *label, uint32 level, bool isVideo, BList *icons, uint32 modifiers=0) 
	: BListItem(level),
	mLabel(label),
	mIsAudioMixer(false),
	mIsVideo(isVideo),
	mIsDefaultInput(false),
	mIsDefaultOutput(false)
{
	mIcons = icons;
	mInfo = NULL;
	
	SetHeight(16 + kITEM_MARGIN);
}

MediaListItem::~MediaListItem()
{
}
//--------------------------------------------------------------------------------------------------------------//


//MediaListItem - DrawItem
void 
MediaListItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	rgb_color kHighlight = { 140,140,140,0 };
	rgb_color kBlack = { 0,0,0,0 };

	BRect r(frame);

	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected()) {
			color = kHighlight;
		} else {
			color = owner->ViewColor();
		}
		owner->SetHighColor(color);
		owner->SetLowColor(color);
		owner->FillRect(r);
		owner->SetHighColor(kBlack);
	} else {
		owner->SetLowColor(owner->ViewColor());
	}
	
	frame.left += 4;
	BRect iconFrame(frame);
	iconFrame.Set(iconFrame.left, iconFrame.top+1, iconFrame.left+15, iconFrame.top+16);
	uint32 index = 0;
	if(OutlineLevel()==0 || (mIsDefaultInput&&mIsDefaultOutput)) {
		if(mIsDefaultInput && mIsVideo)
			index = 4;
		else if(mIsDefaultInput && !mIsVideo)
			index = 2;
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(static_cast<BBitmap*>(mIcons->ItemAt(index)), iconFrame);
		owner->SetDrawingMode(B_OP_COPY);
	}
	iconFrame.OffsetBy(16, 0);
	if(mIsDefaultInput || mIsDefaultOutput || mIsAudioMixer) {
		if(mIsAudioMixer)
			index = 1;
		else if(mIsDefaultOutput) {
			if(mIsVideo)
				index = 5;
			else
				index = 3;
		} else {
			if(mIsVideo)
				index = 4;
			else
				index = 2;
		}
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(static_cast<BBitmap*>(mIcons->ItemAt(index)), iconFrame);
		owner->SetDrawingMode(B_OP_COPY);
	}

	frame.left += 16 * (OutlineLevel() + 1);
	owner->SetHighColor(kBlack);
	
	BFont		font = be_plain_font;
	font_height	finfo;
	font.GetHeight(&finfo);
	owner->SetFont(&font);
	owner->MovePenTo(frame.left+8, frame.top + ((frame.Height() - (finfo.ascent + finfo.descent + finfo.leading)) / 2) +
					(finfo.ascent + finfo.descent) - 1);
	owner->DrawString(mLabel);
}
//--------------------------------------------------------------------------------------------------------------//

void 
MediaListItem::SetDefault(bool isDefault, bool isInput)
{
	if(isInput)
		mIsDefaultInput = isDefault;
	else
		mIsDefaultOutput = isDefault;
}

void 
MediaListItem::SetAudioMixer(bool isAudioMixer)
{
	mIsAudioMixer = isAudioMixer;
}

void 
MediaListItem::Update(BView *owner, const BFont *finfo)
{
	// we need to override the update method so we can make sure are
	// list item size doesn't change
	BListItem::Update(owner, finfo);
	if ((Height() < 16 + kITEM_MARGIN)) {
		SetHeight(16 + kITEM_MARGIN);
	}
}

int 
MediaListItem::Compare(const void *firstArg, const void *secondArg)
{
	const MediaListItem *item1 = *static_cast<const MediaListItem * const *>(firstArg);
	const MediaListItem *item2 = *static_cast<const MediaListItem * const *>(secondArg);
	if(item1->mIsVideo != item2->mIsVideo)
		return item1->mIsVideo ? 1 : -1;
	if(item1->OutlineLevel()!=item2->OutlineLevel())
		return item1->OutlineLevel()>item2->OutlineLevel() ? 1 : -1;
	if(item1->mIsAudioMixer!=item2->mIsAudioMixer)
		return item2->mIsAudioMixer ? 1 : -1;
	return strcmp(item1->mLabel, item2->mLabel);
}
