/* 
 * Copyright 2005, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <ChannelControl.h>
#include <PropertyInfo.h>


static property_info // TODO: Finish this
sPropertyInfo[] = {	
	{0}
};


BChannelControl::BChannelControl(BRect frame, const char *name, const char *label,
	BMessage *model, int32 channel_count, uint32 resizeMode, uint32 flags)
	: BControl(frame, name, label, model, resizeMode, flags),
	fChannelCount(channel_count),
	fCurrentChannel(0),
	fChannelMin(NULL),
	fChannelMax(NULL),
	fChannelValues(NULL),
	fMultiLabels(NULL),
	fModificationMsg(NULL)
{
	fChannelMin = new int32[channel_count];
	memset(fChannelMin, 0, sizeof(int32) * channel_count);
	
	fChannelMax = new int32[channel_count];
	memset(fChannelMax, 100, sizeof(int32) * channel_count);
	
	fChannelValues = new int32[channel_count];
	memset(fChannelValues, 0, sizeof(int32) * channel_count);
}


BChannelControl::BChannelControl(BMessage *archive)
	: BControl(archive)
{
}


BChannelControl::~BChannelControl()
{
	delete[] fChannelMin;
	delete[] fChannelMax;
	delete[] fChannelValues;
}


status_t
BChannelControl::Archive(BMessage *into, bool deep) const
{
	return B_ERROR;
}


void
BChannelControl::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
}


void
BChannelControl::SetFont(const BFont *font, uint32 mask)
{
	BView::SetFont(font, mask);
}


void
BChannelControl::AttachedToWindow()
{
	BControl::AttachedToWindow();
}


void
BChannelControl::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}


void
BChannelControl::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
}


void
BChannelControl::MessageReceived(BMessage *message)
{
	BControl::MessageReceived(message);
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
	if (data == NULL)
		return B_BAD_VALUE;
	
	data->AddString("suites", "suite/vnd.Be-channel-control");

	BPropertyInfo propertyInfo(sPropertyInfo);
	data->AddFlat("messages", &propertyInfo);

	return BControl::GetSupportedSuites(data);
}


void
BChannelControl::SetModificationMessage(BMessage *message)
{
	delete fModificationMsg;
	fModificationMsg = message;
}


BMessage *
BChannelControl::ModificationMessage() const
{
	return fModificationMsg;
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
	BeginInvokeNotify(kind);
	status_t status = InvokeChannel(msg, fromChannel, channelCount, inMask);
	EndInvokeNotify();
	
	return status;
}


void
BChannelControl::SetValue(int32 value)
{
	// Get real
	if (value > fChannelMax[fCurrentChannel])
		value = fChannelMax[fCurrentChannel];
	
	if (value < fChannelMin[fCurrentChannel]);
		value = fChannelMin[fCurrentChannel];
		
	if (value != fChannelValues[fCurrentChannel]) {
		StuffValues(fCurrentChannel, 1, &value);
		BControl::SetValue(value);
	}
}


status_t
BChannelControl::SetCurrentChannel(int32 channel)
{
	if (channel < 0 || channel >= fChannelCount)
		return B_BAD_INDEX;
		
	if (channel != fCurrentChannel) {
		fCurrentChannel = channel;
		BControl::SetValue(fChannelValues[fCurrentChannel]);
	}
	
	return B_OK;	
}


int32
BChannelControl::CurrentChannel() const
{
	return fCurrentChannel;
}


int32
BChannelControl::CountChannels() const
{
	return fChannelCount;
}


status_t
BChannelControl::SetChannelCount(int32 channel_count)
{
	if (channel_count < 0 || channel_count >= MaxChannelCount())
		return B_BAD_VALUE;
	
	// TODO: Currently we only grow the buffer. Test what BeOS does
	if (channel_count > fChannelCount) {
		int32 *newMin = new int32[channel_count];
		int32 *newMax = new int32[channel_count];
		int32 *newVal = new int32[channel_count];
		
		memcpy(newMin, fChannelMin, fChannelCount);
		memcpy(newMax, fChannelMax, fChannelCount);
		memcpy(newVal, fChannelValues, fChannelCount);
		
		delete[] fChannelMin;
		delete[] fChannelMax;
		delete[] fChannelValues;
		
		fChannelMin = newMin;
		fChannelMax = newMax;
		fChannelValues = newVal;
	}
	
	fChannelCount = channel_count;
	
	return B_OK;
}


int32
BChannelControl::ValueFor(int32 channel) const
{
	int32 value = 0;
	if (GetValue(&value, channel, 1) <= 0)
		return -1;
	
	return value;
}


int32
BChannelControl::GetValue(int32 *outValues, int32 fromChannel,
	int32 channelCount) const
{
	int32 i = 0;
	for (i = 0; i < channelCount; i++)
		outValues[i] = fChannelValues[fromChannel + i];

	return i;
}


status_t
BChannelControl::SetValueFor(int32 channel, int32 value)
{
	return SetValue(channel, 1, &value);
}


status_t
BChannelControl::SetValue(int32 fromChannel, int32 channelCount,
	const int32 *inValues)
{
	return StuffValues(fromChannel, channelCount, inValues);
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
