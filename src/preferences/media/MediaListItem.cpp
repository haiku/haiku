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
#include <string.h>

#include <View.h>

#include "MediaListItem.h"

#define kITEM_MARGIN					  1

// MediaListItem - Constructor
MediaListItem::MediaListItem(dormant_node_info *info, uint32 level, bool isVideo, BList *icons, uint32 modifiers) 
	: BListItem(level),
	fIsAudioMixer(false),
	fIsVideo(isVideo),
	fIsDefaultInput(false),
	fIsDefaultOutput(false)
{
	fIcons = icons;
	fInfo = info;
	fLabel = fInfo->name;
	
	SetHeight(16 + kITEM_MARGIN);
}

MediaListItem::MediaListItem(const char *label, uint32 level, bool isVideo, BList *icons, uint32 modifiers) 
	: BListItem(level),
	fLabel(label),
	fIsAudioMixer(false),
	fIsVideo(isVideo),
	fIsDefaultInput(false),
	fIsDefaultOutput(false)
{
	fIcons = icons;
	fInfo = NULL;
	
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
	if(OutlineLevel()==0 || (fIsDefaultInput && fIsDefaultOutput)) {
		if(fIsDefaultInput && fIsVideo)
			index = 4;
		else if(fIsDefaultInput && !fIsVideo)
			index = 2;
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(static_cast<BBitmap*>(fIcons->ItemAt(index)), iconFrame);
		owner->SetDrawingMode(B_OP_COPY);
	}
	iconFrame.OffsetBy(16, 0);
	if(fIsDefaultInput || fIsDefaultOutput || fIsAudioMixer) {
		if(fIsAudioMixer)
			index = 1;
		else if(fIsDefaultOutput) {
			if(fIsVideo)
				index = 5;
			else
				index = 3;
		} else {
			if(fIsVideo)
				index = 4;
			else
				index = 2;
		}
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(static_cast<BBitmap*>(fIcons->ItemAt(index)), iconFrame);
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
	owner->DrawString(fLabel);
}
//--------------------------------------------------------------------------------------------------------------//

void 
MediaListItem::SetDefault(bool isDefault, bool isInput)
{
	if(isInput)
		fIsDefaultInput = isDefault;
	else
		fIsDefaultOutput = isDefault;
}

void 
MediaListItem::SetAudioMixer(bool isAudioMixer)
{
	fIsAudioMixer = isAudioMixer;
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
	if(item1->fIsVideo != item2->fIsVideo)
		return item1->fIsVideo ? 1 : -1;
	if(item1->OutlineLevel()!=item2->OutlineLevel())
		return item1->OutlineLevel()>item2->OutlineLevel() ? 1 : -1;
	if(item1->fIsAudioMixer!=item2->fIsAudioMixer)
		return item2->fIsAudioMixer ? 1 : -1;
	return strcmp(item1->fLabel, item2->fLabel);
}
