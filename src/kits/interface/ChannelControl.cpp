/* 
 * Copyright 2005, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <ChannelControl.h>
#include <Message.h>


BChannelControl::BChannelControl(BRect frame, const char *name, const char *label,
	BMessage *model, int32 channel_count, uint32 resizeMode, uint32 flags)
	: BControl(frame, name, label, model, resizeMode, flags)
{
}


BChannelControl::BChannelControl(BMessage *archive)
	: BControl(archive)
{
}


BChannelControl::~BChannelControl()
{
}


status_t
BChannelControl::Archive(BMessage *into, bool deep) const
{
	return B_ERROR;
}


void
BChannelControl::FrameResized(float width, float height)
{
}


void
BChannelControl::SetFont(const BFont *font, uint32 mask)
{
}


void
BChannelControl::AttachedToWindow()
{
}


void
BChannelControl::DetachedFromWindow()
{
}


void
BChannelControl::ResizeToPreferred()
{
}


void
BChannelControl::MessageReceived(BMessage *message)
{
}


BHandler *
BChannelControl::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,
	int32 form, const char *property)
{
	return NULL;
}


status_t
BChannelControl::GetSupportedSuites(BMessage *data)
{
	return B_ERROR;
}


void
BChannelControl::SetModificationMessage(BMessage *message)
{
}


BMessage *
BChannelControl::ModificationMessage() const
{
	return NULL;
}


status_t
BChannelControl::Invoke(BMessage *msg)
{
	return B_ERROR;
}


status_t
BChannelControl::InvokeChannel(BMessage *msg, int32 fromChannel,
	int32 channelCount, const bool *inMask)
{
	return B_ERROR;
}


status_t
BChannelControl::InvokeNotifyChannel(BMessage *msg, uint32 kind,
	int32 fromChannel, int32 channelCount, const bool *inMask)
{
	return B_ERROR;
}


void
BChannelControl::SetValue(int32 value)
{
}


status_t
BChannelControl::SetCurrentChannel(int32 channel)
{
	return B_ERROR;
}


int32
BChannelControl::CurrentChannel() const
{
	return -1;
}


int32
BChannelControl::CountChannels() const
{
	return -1;
}


status_t
BChannelControl::SetChannelCount(int32 channel_count)
{
	return B_ERROR;
}


int32
BChannelControl::ValueFor(int32 channel) const
{
	return -1;
}


int32
BChannelControl::GetValue(int32 *outValues, int32 fromChannel,
	int32 channelCount) const
{
	return -1;
}


status_t
BChannelControl::SetValueFor(int32 channel, int32 value)
{
	return B_ERROR;
}


status_t
BChannelControl::SetValue(int32 fromChannel, int32 channelCount,
	const int32 *inValues)
{
	return B_ERROR;
}


status_t
BChannelControl::SetAllValue(int32 values)
{
	return B_ERROR;
}


status_t
BChannelControl::SetLimitsFor(int32 channel, int32 minimum, int32 maximum)
{
	return B_ERROR;
}


status_t
BChannelControl::GetLimitsFor(int32 channel, int32 *minimum, int32 *maximum) const
{
	return B_ERROR;
}


status_t
BChannelControl::SetLimitsFor(int32 fromChannel, int32 channelCount,
	const int32 *minimum, const int32 *maximum)
{
	return B_ERROR;
}


status_t
BChannelControl::GetLimitsFor(int32 fromChannel, int32 channelCount,
	int32 *minimum, int32 *maximum) const
{
	return B_ERROR;
}


status_t
BChannelControl::SetLimits(int32 minimum, int32 maximum)
{
	return B_ERROR;
}


status_t
BChannelControl::GetLimits(int32 *outMinimum, int32 *outMaximum) const
{
	return B_ERROR;
}


status_t
BChannelControl::SetLimitLabels(const char *min_label, const char *max_label)
{
	return B_ERROR;
}


const char *
BChannelControl::MinLimitLabel() const
{
	return NULL;
}


const char *
BChannelControl::MaxLimitLabel() const
{
	return NULL;
}


status_t
BChannelControl::SetLimitLabelsFor(int32 channel, const char *minLabel, const char *maxLabel)
{
	return B_ERROR;
}


status_t
BChannelControl::SetLimitLabelsFor(int32 from_channel, int32 channel_count, const char *minLabel, const char *maxLabel)
{
	return B_ERROR;
}


const char *
BChannelControl::MinLimitLabelFor(int32 channel) const
{
	return NULL;
}


const char *
BChannelControl::MaxLimitLabelFor(int32 channel) const
{
	return NULL;
}


status_t 
BChannelControl::StuffValues(int32 fromChannel, int32 channelCount,
	const int32 *inValues)
{
	return B_ERROR;
}


void BChannelControl::_Reserverd_ChannelControl_0(void *, ...) {}
void BChannelControl::_Reserverd_ChannelControl_1(void *, ...) {}
void BChannelControl::_Reserverd_ChannelControl_2(void *, ...) {}
void BChannelControl::_Reserverd_ChannelControl_3(void *, ...) {}
void BChannelControl::_Reserverd_ChannelControl_4(void *, ...) {}
void BChannelControl::_Reserverd_ChannelControl_5(void *, ...) {}
void BChannelControl::_Reserverd_ChannelControl_6(void *, ...) {}
void BChannelControl::_Reserverd_ChannelControl_7(void *, ...) {}
void BChannelControl::_Reserverd_ChannelControl_8(void *, ...) {}
void BChannelControl::_Reserverd_ChannelControl_9(void *, ...) {}
void BChannelControl::_Reserverd_ChannelControl_10(void *, ...) {}
void BChannelControl::_Reserverd_ChannelControl_11(void *, ...) {}

