/*
	
	SettingsView.h
*/

#ifndef SETTINGS_VIEW_H
#define SETTINGS_VIEW_H

#ifndef _VIEW_H
#include <View.h>
#endif

#include "CalendarView.h"
#include "AnalogClock.h"
#include "DateTimeEdit.h"

class TSettingsView:public BView
{
	public:
		TSettingsView(BRect frame);
		virtual void AttachedToWindow();
		virtual void Draw(BRect frame);
		virtual void MessageReceived(BMessage *message);
		
		void ChangeRTCSetting();
	private:
		void InitView();
		void ReadFiles(); // reads RTC_time_settings
		void UpdateDateTime(BMessage *message);
		
		BRect f_temp;
		BRadioButton	*f_local;
		BRadioButton	*f_gmt;
		TDateEdit		*f_dateedit;
		TTimeEdit		*f_timeedit;
		TCalendarView 	*f_calendar;
		TAnalogClock	*f_clock;
		bool 			f_islocal; // local or gmt?
};

#endif
