/*
 * Copyright 2008-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CHANNEL_CONTROL_H
#define _CHANNEL_CONTROL_H

//! BChannelControl is the base class for controls that have several
// independent values, with minima and maxima.

#include <Control.h>
#include <String.h>

class BChannelControl : public BControl {
public:
								BChannelControl(BRect frame, const char* name,
									const char* label, BMessage* model,
									int32 channelCount = 1,
									uint32 resizeFlags
										= B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 viewFlags = B_WILL_DRAW);
								BChannelControl(const char* name,
									const char* label, BMessage* model,
									int32 channelCount = 1,
									uint32 viewFlags = B_WILL_DRAW);
								BChannelControl(BMessage* archive);
	virtual						~BChannelControl();

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;

	virtual	void				Draw(BRect updateRect) = 0;
	virtual	void				MouseDown(BPoint where) = 0;
	virtual	void				KeyDown(const char* bytes, int32 size) = 0;

	virtual	void				FrameResized(float width, float height);
	virtual	void				SetFont(const BFont* font,
									uint32 mask = B_FONT_ALL);

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				ResizeToPreferred();
	virtual	void				GetPreferredSize(float* width,
									float* height) = 0;

	virtual	void				MessageReceived(BMessage* message);

	virtual	BHandler*			ResolveSpecifier(BMessage* message, int32 index,
									BMessage* specifier, int32 form,
									const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

	virtual	void				SetModificationMessage(BMessage* message);
			BMessage*			ModificationMessage() const;
		
	virtual	status_t			Invoke(BMessage* withMessage = NULL);

	//! These methods are similar to Invoke() Invoke() and InvokeNotify(), but
	// include additional information about all of the channels in the control.
	virtual	status_t			InvokeChannel(BMessage* message = NULL,
									int32 fromChannel = 0,
									int32 channelCount = -1,
									const bool* _mask = NULL);
			status_t			InvokeNotifyChannel(BMessage* message = NULL,
									uint32 kind = B_CONTROL_INVOKED,
									int32 fromChannel = 0,
									int32 channelCount = -1,
									const bool* _mask = NULL);

	virtual	void				SetValue(int32 value);
		// SetCurrentChannel() determines which channel
	virtual	status_t			SetCurrentChannel(int32 index);
			int32				CurrentChannel() const;

	virtual	int32				CountChannels() const;
	virtual	int32				MaxChannelCount() const = 0;
	virtual	status_t			SetChannelCount(int32 count);
			int32				ValueFor(int32 channel) const;
	virtual	int32				GetValue(int32* _values, int32 fromChannel,
									int32 channelCount) const;
			status_t			SetValueFor(int32 channel, int32 value);
	virtual	status_t			SetValue(int32 fromChannel, int32 channelCount,
									const int32* values);
			status_t			SetAllValue(int32 values);
			status_t			SetLimitsFor(int32 channel, int32 minimum,
									int32 maximum);
			status_t			GetLimitsFor(int32 channel, int32* _minimum,
									int32* _maximum) const ;
	virtual	status_t			SetLimitsFor(int32 fromChannel,
									int32 channelCount, const int32* minima,
									const int32* maxima);
	virtual	status_t			GetLimitsFor(int32 fromChannel,
									int32 channelCount, int32* minima,
									int32* maxima) const;
			status_t			SetLimits(int32 minimum, int32 maximum);
			status_t			GetLimits(int32* _minimum,
									int32* _maximum) const;

	virtual	bool				SupportsIndividualLimits() const = 0;
	virtual	status_t			SetLimitLabels(const char* minLabel,
									const char* maxLabel);
			const char* 		MinLimitLabel() const;
			const char* 		MaxLimitLabel() const;
	virtual	status_t			SetLimitLabelsFor(int32 channel,
									const char* minLabel, const char* maxLabel);
	virtual	status_t			SetLimitLabelsFor(int32 fromChannel,
									int32 channelCount, const char* minLabel,
									const char* maxLabel);
			const char*			MinLimitLabelFor(int32 channel) const;
			const char*			MaxLimitLabelFor(int32 channel) const;
private:
	// Forbidden (and unimplemented)
								BChannelControl(const BChannelControl& other);
			BChannelControl&	operator=(const BChannelControl& other);

	virtual	void				_Reserverd_ChannelControl_0(void *, ...);
	virtual	void				_Reserverd_ChannelControl_1(void *, ...);
	virtual	void				_Reserverd_ChannelControl_2(void *, ...);
	virtual	void				_Reserverd_ChannelControl_3(void *, ...);
	virtual	void				_Reserverd_ChannelControl_4(void *, ...);
	virtual	void				_Reserverd_ChannelControl_5(void *, ...);
	virtual	void				_Reserverd_ChannelControl_6(void *, ...);
	virtual	void				_Reserverd_ChannelControl_7(void *, ...);
	virtual	void				_Reserverd_ChannelControl_8(void *, ...);
	virtual	void				_Reserverd_ChannelControl_9(void *, ...);
	virtual	void				_Reserverd_ChannelControl_10(void *, ...);
	virtual	void				_Reserverd_ChannelControl_11(void *, ...);

protected:
	inline	int32* const&		MinLimitList() const;
	inline	int32* const&		MaxLimitList() const;
	inline	int32* const&		ValueList() const;

private:
			int32				fChannelCount;
			int32				fCurrentChannel;
			int32*				fChannelMin;
			int32*				fChannelMax;
			int32*				fChannelValues;

			BString				fMinLabel;
			BString				fMaxLabel;

			void*				fMultiLabels;

			BMessage*			fModificationMsg;

			uint32				_reserved_[15];

			status_t			StuffValues(int32 fromChannel,
									int32 channelCount, const int32* values);
};


inline int32* const&
BChannelControl::MinLimitList() const
{
	return fChannelMin;
}


inline int32* const&
BChannelControl::MaxLimitList() const
{
	return fChannelMax;
}


inline int32* const&
BChannelControl::ValueList() const
{
	return fChannelValues;
}


#endif // _CHANNEL_CONTROL_H
