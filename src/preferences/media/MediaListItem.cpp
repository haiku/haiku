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
#include "MediaListItem.h"

#include <string.h>

#include <MediaAddOn.h>
#include <View.h>

#include "MediaIcons.h"
#include "MediaWindow.h"


#define kITEM_MARGIN	1
#define GREATER_THAN	-1
#define LESS_THAN		1


MediaIcons* MediaListItem::sIcons = NULL;


struct MediaListItem::Renderer {
	Renderer()
		:
		fTitle(NULL),
		fPrimaryIcon(NULL),
		fSecondaryIcon(NULL),
		fDoubleInsets(true),
		fSelected(false)
	{
	}

	// The first icon added is drawn next to the label,
	// the second is drawn to the right of the label.
	void AddIcon(BBitmap* icon)
	{
		if (!fPrimaryIcon)
			fPrimaryIcon = icon;
		else {
			fSecondaryIcon = fPrimaryIcon;
			fPrimaryIcon = icon;
		}
	}

	void SetTitle(const char* title)
	{
		fTitle = title;
	}

	void SetSelected(bool selected)
	{
		fSelected = selected;
	}

	// set whether or not to leave enough room for two icons,
	// defaults to true.
	void UseDoubleInset(bool doubleInset)
	{
		fDoubleInsets = doubleInset;
	}

	void Render(BView* onto, BRect frame, bool complete = false)
	{
		const rgb_color lowColor = onto->LowColor();
		const rgb_color highColor = onto->HighColor();
		const rgb_color kBlack = {0, 0, 0, 255};

		if (fSelected || complete) {
			if (fSelected)
				onto->SetLowColor(tint_color(lowColor, B_DARKEN_2_TINT));
			onto->FillRect(frame, B_SOLID_LOW);
		}

		frame.left += 4;
		frame.top += kITEM_MARGIN;
		BRect iconFrame(MediaIcons::IconRectAt(frame.LeftTop() + BPoint(1, 0)));

		onto->SetDrawingMode(B_OP_OVER);
		if (fPrimaryIcon && !fDoubleInsets) {
			onto->DrawBitmap(fPrimaryIcon, iconFrame);
			frame.left = iconFrame.right + 1;
		} else if (fSecondaryIcon) {
			onto->DrawBitmap(fSecondaryIcon, iconFrame);
		}
		iconFrame = MediaIcons::IconRectAt(iconFrame.RightTop() + BPoint(1, 0));

		if (fDoubleInsets && fPrimaryIcon) {
			onto->DrawBitmap(fPrimaryIcon, iconFrame);
			frame.left = iconFrame.right + 1;
		}

		onto->SetDrawingMode(B_OP_COPY);
		onto->SetHighColor(kBlack);

		BFont font = be_plain_font;
		font_height	fontInfo;
		font.GetHeight(&fontInfo);
		float lineHeight = fontInfo.ascent + fontInfo.descent
			+ fontInfo.leading;
		onto->SetFont(&font);
		onto->MovePenTo(frame.left + 8, frame.top
			+ ((frame.Height() - (lineHeight)) / 2)
			+ (fontInfo.ascent + fontInfo.descent) - 1);
		onto->DrawString(fTitle);

		onto->SetHighColor(highColor);
		onto->SetLowColor(lowColor);
	}

	float ItemWidth()
	{
		float width = 4.0f;
			// left margin

		float iconSpace = MediaIcons::sBounds.Width() + 1.0f;
		if (fDoubleInsets)
			iconSpace *= 2.0f;
		width += iconSpace;
		width += 8.0f;
			// space between icons and text

		width += be_plain_font->StringWidth(fTitle) + 16.0f;
		return width;
	}

private:

	const char*	fTitle;
	BBitmap*	fPrimaryIcon;
	BBitmap*	fSecondaryIcon;
	bool		fDoubleInsets;
	bool		fSelected;
};


MediaListItem::MediaListItem()
	:
	BListItem((uint32)0)
{
}


void
MediaListItem::Update(BView* owner, const BFont* font)
{
	// we need to override the update method so we can make sure our
	// list item size doesn't change
	BListItem::Update(owner, font);

	float iconHeight = MediaIcons::sBounds.Height() + 1;
	if ((Height() < iconHeight + kITEM_MARGIN * 2)) {
		SetHeight(iconHeight + kITEM_MARGIN * 2);
	}

	Renderer renderer;
	renderer.SetTitle(Label());
	SetRenderParameters(renderer);
	SetWidth(renderer.ItemWidth());
}


void
MediaListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	Renderer renderer;
	renderer.SetSelected(IsSelected());
	renderer.SetTitle(Label());
	SetRenderParameters(renderer);
	renderer.Render(owner, frame, complete);
}


int
MediaListItem::Compare(const void* itemOne, const void* itemTwo)
{
	MediaListItem* firstItem = *(MediaListItem**)itemOne;
	MediaListItem* secondItem = *(MediaListItem**)itemTwo;

	return firstItem->CompareWith(secondItem);
}


// #pragma mark - NodeListItem


NodeListItem::NodeListItem(const dormant_node_info* node, media_type type)
	:
	MediaListItem(),
	fNodeInfo(node),
	fMediaType(type),
	fIsDefaultInput(false),
	fIsDefaultOutput(false)
{
}


void
NodeListItem::SetRenderParameters(MediaListItem::Renderer& renderer)
{
	MediaIcons::IconSet* iconSet = &Icons()->videoIcons;
	if (fMediaType == MediaListItem::AUDIO_TYPE)
		iconSet = &Icons()->audioIcons;

	if (fIsDefaultInput)
		renderer.AddIcon(&iconSet->inputIcon);
	if (fIsDefaultOutput)
		renderer.AddIcon(&iconSet->outputIcon);
}


const char*
NodeListItem::Label()
{
	return fNodeInfo->name;
}


void
NodeListItem::SetMediaType(media_type type)
{
	fMediaType = type;
}


void
NodeListItem::SetDefaultOutput(bool isDefault)
{
	fIsDefaultOutput = isDefault;
}


void
NodeListItem::SetDefaultInput(bool isDefault)
{
	fIsDefaultInput = isDefault;
}


void
NodeListItem::AlterWindow(MediaWindow* window)
{
	window->SelectNode(fNodeInfo);
}


void
NodeListItem::Accept(MediaListItem::Visitor& visitor)
{
	visitor.Visit(this);
}


int
NodeListItem::CompareWith(MediaListItem* item)
{
	Comparator comparator(this);
	item->Accept(comparator);
	return comparator.result;
}


NodeListItem::Comparator::Comparator(NodeListItem* compareOthersTo)
	:
	result(GREATER_THAN),
	fTarget(compareOthersTo)
{
}


void
NodeListItem::Comparator::Visit(NodeListItem* item)
{
	result = GREATER_THAN;

	if (fTarget->Type() != item->Type() && fTarget->Type() == VIDEO_TYPE)
		result = LESS_THAN;
	else
		result = strcmp(fTarget->Label(), item->Label());
}


void
NodeListItem::Comparator::Visit(DeviceListItem* item)
{
	result = LESS_THAN;
	if (fTarget->Type() != item->Type() && fTarget->Type() == AUDIO_TYPE)
		result = GREATER_THAN;
}


void
NodeListItem::Comparator::Visit(AudioMixerListItem* item)
{
	result = LESS_THAN;
}


// #pragma mark - DeviceListItem


DeviceListItem::DeviceListItem(const char* title,
	MediaListItem::media_type type)
	:
	MediaListItem(),
	fTitle(title),
	fMediaType(type)
{
}


void
DeviceListItem::Accept(MediaListItem::Visitor& visitor)
{
	visitor.Visit(this);
}


int
DeviceListItem::CompareWith(MediaListItem* item)
{
	Comparator comparator(this);
	item->Accept(comparator);
	return comparator.result;
}


DeviceListItem::Comparator::Comparator(DeviceListItem* compareOthersTo)
	:
	result(GREATER_THAN),
	fTarget(compareOthersTo)
{
}


void
DeviceListItem::Comparator::Visit(NodeListItem* item)
{
	result = GREATER_THAN;
	if (fTarget->Type() != item->Type() && fTarget->Type() == AUDIO_TYPE)
		result = LESS_THAN;
}


void
DeviceListItem::Comparator::Visit(DeviceListItem* item)
{
	result = LESS_THAN;
	if (fTarget->Type() == AUDIO_TYPE)
		result = GREATER_THAN;
}


void
DeviceListItem::Comparator::Visit(AudioMixerListItem* item)
{
	result = LESS_THAN;
	if (fTarget->Type() == AUDIO_TYPE)
		result = GREATER_THAN;
}


void
DeviceListItem::SetRenderParameters(Renderer& renderer)
{
	renderer.AddIcon(&Icons()->devicesIcon);
	renderer.UseDoubleInset(false);
}


void
DeviceListItem::AlterWindow(MediaWindow* window)
{
	if (fMediaType == MediaListItem::AUDIO_TYPE)
		window->SelectAudioSettings(fTitle);
	else
		window->SelectVideoSettings(fTitle);
}


// #pragma mark - AudioMixerListItem


AudioMixerListItem::AudioMixerListItem(const char* title)
	:
	MediaListItem(),
	fTitle(title)
{
}


void
AudioMixerListItem::AlterWindow(MediaWindow* window)
{
	window->SelectAudioMixer(fTitle);
}


void
AudioMixerListItem::Accept(MediaListItem::Visitor& visitor)
{
	visitor.Visit(this);
}


int
AudioMixerListItem::CompareWith(MediaListItem* item)
{
	Comparator comparator(this);
	item->Accept(comparator);
	return comparator.result;
}


AudioMixerListItem::Comparator::Comparator(AudioMixerListItem* compareOthersTo)
	:
	result(0),
	fTarget(compareOthersTo)
{
}


void
AudioMixerListItem::Comparator::Visit(NodeListItem* item)
{
	result = GREATER_THAN;
}


void
AudioMixerListItem::Comparator::Visit(DeviceListItem* item)
{
	result = GREATER_THAN;
	if (item->Type() == AUDIO_TYPE)
		result = LESS_THAN;
}


void
AudioMixerListItem::Comparator::Visit(AudioMixerListItem* item)
{
	result = 0;
}


void
AudioMixerListItem::SetRenderParameters(Renderer& renderer)
{
	renderer.AddIcon(&Icons()->mixerIcon);
}
