/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unkown>
 *		Julun <host.haiku@gmx.de>
 */
#ifndef SETTINGS_VIEW_H
#define SETTINGS_VIEW_H


#include <View.h>


class TDateEdit;
class TTimeEdit;
class TCalendarView;
class TAnalogClock;
class BRadioButton;
class BStringView;

class TSettingsView : public BView {
	public:
						TSettingsView(BRect frame);
		virtual 		~TSettingsView();
		
		virtual void 	AttachedToWindow();
		virtual void 	Draw(BRect updaterect);
		virtual void 	MessageReceived(BMessage *message);
		virtual void	GetPreferredSize(float *width, float *height);

		void 			ChangeRTCSetting();		
		bool 			GMTime();
		void			SetTimeZone(const char *timezone);

	private:
		void 			InitView();
		void 			ReadRTCSettings();
		void 			UpdateDateTime(BMessage *message);

		BRadioButton 	*fLocalTime;
		BRadioButton 	*fGmtTime;
		TDateEdit 		*fDateEdit;
		TTimeEdit 		*fTimeEdit;
		TCalendarView 	*fCalendar;
		TAnalogClock 	*fClock;
		bool 			fIsLocalTime;
		BStringView		*fTimeZone;
};

#endif

