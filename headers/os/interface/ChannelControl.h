/*******************************************************************************
/
/	File:			ChannelControl.h
/
/   Description:    BChannelControl is the base class for controls that
/					have several independent values, with minima and maxima.
/
/	Copyright 1998-99, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_CHANNEL_CONTROL_H)
#define _CHANNEL_CONTROL_H

#include <Control.h>
#include <String.h>

class BChannelControl :
	public BControl
{
public:
							BChannelControl(
									BRect frame,
									const char * name,
									const char * label,
									BMessage * model,
									int32 channel_count = 1,
									uint32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW);
							BChannelControl(
									BMessage * from);
virtual						~BChannelControl();
virtual	status_t			Archive(
									BMessage * into,
									bool deep = true) const;

virtual	void				Draw(
									BRect area) = 0;
virtual	void				MouseDown(
									BPoint where) = 0;
virtual	void				KeyDown(
									const char * bytes,
									int32 size) = 0;

virtual	void				FrameResized(
									float width,
									float height);
virtual	void				SetFont(
									const BFont * font,
									uint32 mask = B_FONT_ALL);

virtual	void				AttachedToWindow();
virtual	void				DetachedFromWindow();
virtual	void				ResizeToPreferred();
virtual	void				GetPreferredSize(
									float * width,
									float * height) = 0;
virtual	void				MessageReceived(
									BMessage * message);

virtual BHandler			*ResolveSpecifier(BMessage *msg,
									int32 index,
									BMessage *specifier,
									int32 form,
									const char *property);
virtual status_t			GetSupportedSuites(BMessage *data);

virtual	void				SetModificationMessage(
									BMessage *message);
		BMessage			*ModificationMessage() const;
		
virtual	status_t			Invoke(BMessage *msg = NULL);

// Perform a full-fledged channel invocation.  These methods are
// just like Invoke() and InvokeNotify(), but include information
// about all of the channels in the control.
virtual	status_t			InvokeChannel(BMessage *msg = NULL,
										  int32 from_channel = 0,
										  int32 channel_count = -1,
										  const bool* in_mask = NULL);
		status_t			InvokeNotifyChannel(BMessage *msg = NULL,
												uint32 kind = B_CONTROL_INVOKED,
												int32 from_channel = 0,
												int32 channel_count = -1,
												const bool* in_mask = NULL);

virtual	void				SetValue(		/* SetCurrentChannel() determines which channel */
									int32 value);
virtual	status_t			SetCurrentChannel(
									int32 channel);
		int32				CurrentChannel() const;

virtual	int32				CountChannels() const;
virtual	int32				MaxChannelCount() const = 0;
virtual	status_t			SetChannelCount(
									int32 channel_count);
		int32				ValueFor(
									int32 channel) const;
virtual	int32				GetValue(
									int32 * out_values,
									int32 from_channel,
									int32 channel_count) const;
		status_t			SetValueFor(
									int32 channel,
									int32 value);
virtual	status_t			SetValue(
									int32 from_channel,
									int32 channel_count,
									const int32 * in_values);
		status_t			SetAllValue(
									int32 values);
		status_t			SetLimitsFor(
									int32 channel,
									int32 minimum,
									int32 maximum);
		status_t			GetLimitsFor(
									int32 channel,
									int32 * minimum,
									int32 * maximum)	const ;
virtual	status_t			SetLimitsFor(
									int32 from_channel,
									int32 channel_count,
									const int32 * minimum,
									const int32 * maximum);
virtual	status_t			GetLimitsFor(
									int32 from_channel,
									int32 channel_count,
									int32 * minimum,
									int32 * maximum) const;
		status_t			SetLimits(
									int32 minimum,
									int32 maximum);
		status_t			GetLimits(
									int32 * outMinimum,
									int32 * outMaximum) const;

virtual	bool				SupportsIndividualLimits() const = 0;
virtual	status_t			SetLimitLabels(
									const char * min_label,
									const char * max_label);
		const char * 		MinLimitLabel() const;
		const char * 		MaxLimitLabel() const;
virtual	status_t			SetLimitLabelsFor(
									int32 channel,
									const char * minLabel,
									const char * maxLabel);
virtual	status_t			SetLimitLabelsFor(
									int32 from_channel,
									int32 channel_count,
									const char * minLabel,
									const char * maxLabel);
		const char *		MinLimitLabelFor(
									int32 channel) const;
		const char *		MaxLimitLabelFor(
									int32 channel) const;
private:

		BChannelControl( /* unimplemented */
				const BChannelControl &);
		BChannelControl & operator=( /* unimplemented */
				const BChannelControl &);
virtual	void _Reserverd_ChannelControl_0(void *, ...);
virtual	void _Reserverd_ChannelControl_1(void *, ...);
virtual	void _Reserverd_ChannelControl_2(void *, ...);
virtual	void _Reserverd_ChannelControl_3(void *, ...);
virtual	void _Reserverd_ChannelControl_4(void *, ...);
virtual	void _Reserverd_ChannelControl_5(void *, ...);
virtual	void _Reserverd_ChannelControl_6(void *, ...);
virtual	void _Reserverd_ChannelControl_7(void *, ...);
virtual	void _Reserverd_ChannelControl_8(void *, ...);
virtual	void _Reserverd_ChannelControl_9(void *, ...);
virtual	void _Reserverd_ChannelControl_10(void *, ...);
virtual	void _Reserverd_ChannelControl_11(void *, ...);

protected:

		inline int32 * const & MinLimitList() const;
		inline int32 * const & MaxLimitList() const;
		inline int32 * const & ValueList() const;

private:

		int32 fChannelCount;
		int32 fCurrentChannel;
		int32 * fChannelMin;
		int32 * fChannelMax;
		int32 * fChannelValues;
		
		BString fMinLabel;
		BString fMaxLabel;
		
		void * fMultiLabels;
		
		BMessage * fModificationMsg;
		
		uint32 _reserved_[15];

		status_t			StuffValues(
									int32 from_channel,
									int32 channel_count,
									const int32 * in_values);
};

inline int32 * const & BChannelControl::MinLimitList() const
{
	return fChannelMin;
}

inline int32 * const & BChannelControl::MaxLimitList() const
{
	return fChannelMax;
}

inline int32 * const & BChannelControl::ValueList() const
{
	return fChannelValues;
}


#endif /* _CHANNEL_CONTROL_H */

