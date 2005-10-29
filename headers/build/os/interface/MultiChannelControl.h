/*******************************************************************************
/
/	File:			MultiChannelControl.h
/
/   Description:    BMultiChannelControl is the base class for controls that
/					have several independent values, with minima and maxima.
/
/	Copyright 1998-99, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_MULTI_CHANNEL_CONTROL_H)
#define _MULTI_CHANNEL_CONTROL_H

#include <Control.h>


class BMultiChannelControl :
	public BControl
{
public:
		BMultiChannelControl(
				BRect frame,
				const char * name,
				const char * label,
				BMessage * model,
				int32 channel_count = 1,
				uint32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW);
		BMultiChannelControl(
				BMessage * from);
virtual	~BMultiChannelControl();
virtual	status_t Archive(
				BMessage * into,
				bool deep = true) const;

virtual	void Draw(
				BRect area) = 0;
virtual	void MouseDown(
				BPoint where) = 0;
virtual	void KeyDown(
				const char * bytes,
				int32 size) = 0;

virtual	void FrameResized(
				float width,
				float height);
virtual	void SetFont(
				const BFont * font,
				uint32 mask = B_FONT_ALL);
virtual	void AttachedToWindow();
virtual	void DetachedFromWindow();
virtual	void ResizeToPreferred();
virtual	void GetPreferredSize(
				float * width,
				float * height) = 0;
virtual	void MessageReceived(
				BMessage * message);

virtual	void SetValue(		/* SetValueChannel() determines which channel */
				int32 value);
virtual	status_t SetCurrentChannel(
				int32 channel);
		int32 CurrentChannel() const;

virtual	int32 CountChannels() const;
virtual	int32 MaxChannelCount() const = 0;
virtual	status_t SetChannelCount(
				int32 channel_count);
		int32 ValueFor(
				int32 channel) const;
virtual	int32 GetValues(
				int32 * out_values,
				int32 from_channel,
				int32 channel_count) const;
		status_t SetValueFor(
				int32 channel,
				int32 value);
virtual	status_t SetValues(
				int32 from_channel,
				int32 channel_count,
				const int32 * in_values);
		status_t SetAllValues(
				int32 values);
		status_t SetLimitsFor(
				int32 channel,
				int32 minimum,
				int32 maximum);
		status_t GetLimitsFor(
				int32 channel,
				int32 * minimum,
				int32 * maximum)	const ;
virtual	status_t SetLimits(
				int32 from_channel,
				int32 channel_count,
				const int32 * minimum,
				const int32 * maximum);
virtual	status_t GetLimits(
				int32 from_channel,
				int32 channel_count,
				int32 * minimum,
				int32 * maximum) const;
		status_t SetAllLimits(
				int32 minimum,
				int32 maximum);

virtual	status_t SetLimitLabels(
				const char * min_label,
				const char * max_label);
		const char * MinLimitLabel() const;
		const char * MaxLimitLabel() const;

private:

		BMultiChannelControl( /* unimplemented */
				const BMultiChannelControl &);
		BMultiChannelControl & operator=( /* unimplemented */
				const BMultiChannelControl &);
virtual	status_t _Reserverd_MultiChannelControl_0(void *, ...);
virtual	status_t _Reserverd_MultiChannelControl_1(void *, ...);
virtual	status_t _Reserverd_MultiChannelControl_2(void *, ...);
virtual	status_t _Reserverd_MultiChannelControl_3(void *, ...);
virtual	status_t _Reserverd_MultiChannelControl_4(void *, ...);
virtual	status_t _Reserverd_MultiChannelControl_5(void *, ...);
virtual	status_t _Reserverd_MultiChannelControl_6(void *, ...);
virtual	status_t _Reserverd_MultiChannelControl_7(void *, ...);

protected:

		inline int32 * const & MinLimitList() const;
		inline int32 * const & MaxLimitList() const;
		inline int32 * const & ValueList() const;

private:

		int32 _m_channel_count;
		int32 _m_value_channel;
		int32 * _m_channel_min;
		int32 * _m_channel_max;
		int32 * _m_channel_val;
		char * _m_min_label;
		char * _m_max_label;

		uint32 _reserved_[16];

};

inline int32 * const & BMultiChannelControl::MinLimitList() const
{
	return _m_channel_min;
}

inline int32 * const & BMultiChannelControl::MaxLimitList() const
{
	return _m_channel_max;
}

inline int32 * const & BMultiChannelControl::ValueList() const
{
	return _m_channel_val;
}


#endif /* _MULTI_CHANNEL_CONTROL_H */

