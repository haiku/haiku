/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MULTI_CHANNEL_CONTROL_H
#define _MULTI_CHANNEL_CONTROL_H


#include <Control.h>


class BMultiChannelControl : public BControl {
public:
								BMultiChannelControl(BRect frame,
									const char* name, const char* label,
									BMessage* message, int32 channelCount = 1,
									uint32 resize = B_FOLLOW_LEFT
										| B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW);
								BMultiChannelControl(BMessage* archive);
	virtual						~BMultiChannelControl();

	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				Draw(BRect updateRect) = 0;
	virtual	void				MouseDown(BPoint where) = 0;
	virtual	void				KeyDown(const char* bytes, int32 numBytes) = 0;

	virtual	void				FrameResized(float width, float height);
	virtual	void				SetFont(const BFont* font,
									uint32 mask = B_FONT_ALL);

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				ResizeToPreferred();
	virtual	void				GetPreferredSize(float* _width,
									float* _height) = 0;

	virtual	void				MessageReceived(BMessage* message);

	//! SetValueChannel() determines which channel
	virtual	void				SetValue(int32 value);
	virtual	status_t			SetCurrentChannel(int32 channel);
			int32				CurrentChannel() const;

	virtual	int32				CountChannels() const;
	virtual	int32				MaxChannelCount() const = 0;
	virtual	status_t			SetChannelCount(int32 channelCount);
			int32				ValueFor(int32 channel) const;
	virtual	int32				GetValues(int32* _values, int32 firstChannel,
									int32 channelCount) const;
			status_t			SetValueFor(int32 channel, int32 value);
	virtual	status_t			SetValues(int32 firstChannel,
									int32 channelCount, const int32* _values);
			status_t			SetAllValues(int32 values);
			status_t			SetLimitsFor(int32 channel, int32 minimum,
									int32 maximum);
			status_t			GetLimitsFor(int32 channel, int32* _minimum,
									int32* _maximum) const;
	virtual	status_t			SetLimits(int32 firstChannel,
									int32 channelCount, const int32* minimum,
									const int32* maximum);
	virtual	status_t			GetLimits(int32 firstChannel,
									int32 channelCount, int32* _minimum,
									int32* _maximum) const;
			status_t			SetAllLimits(int32 minimum, int32 maximum);

	virtual	status_t			SetLimitLabels(const char* minLabel,
									const char* maxLabel);
			const char*			MinLimitLabel() const;
			const char*			MaxLimitLabel() const;

private:
	// FBC padding
	virtual	status_t			_Reserverd_MultiChannelControl_0(void*, ...);
	virtual	status_t			_Reserverd_MultiChannelControl_1(void*, ...);
	virtual	status_t			_Reserverd_MultiChannelControl_2(void*, ...);
	virtual	status_t			_Reserverd_MultiChannelControl_3(void*, ...);
	virtual	status_t			_Reserverd_MultiChannelControl_4(void*, ...);
	virtual	status_t			_Reserverd_MultiChannelControl_5(void*, ...);
	virtual	status_t			_Reserverd_MultiChannelControl_6(void*, ...);
	virtual	status_t			_Reserverd_MultiChannelControl_7(void*, ...);

	// Forbidden
								BMultiChannelControl(
									const BMultiChannelControl&);
			BMultiChannelControl& operator=(const BMultiChannelControl&);


protected:
	inline	int32* const&		MinLimitList() const;
	inline	int32* const&		MaxLimitList() const;
	inline	int32* const&		ValueList() const;

private:
			int32				fChannelCount;
			int32				fValueChannel;
			int32*				fChannelMinima;
			int32*				fChannelMaxima;
			int32*				fChannelValues;
			char*				fMinLabel;
			char*				fMaxLabel;

			uint32				_reserved_[16];
};


inline int32* const&
BMultiChannelControl::MinLimitList() const
{
	return fChannelMinima;
}


inline int32* const&
BMultiChannelControl::MaxLimitList() const
{
	return fChannelMaxima;
}


inline int32* const&
BMultiChannelControl::ValueList() const
{
	return fChannelValues;
}


#endif // _MULTI_CHANNEL_CONTROL_H

