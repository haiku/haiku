/*
 * Copyright 2008 Karsten Heimrich, host.haiku@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CALENDAR_MENU_WINDOW_H_
#define _CALENDAR_MENU_WINDOW_H_


#include <DateTime.h>
#include <Window.h>


class BMessage;
class BStringView;

namespace BPrivate {
	class BCalendarView;
}

using BPrivate::BCalendarView;

class CalendarMenuWindow : public BWindow {
public:
								CalendarMenuWindow(BPoint where);
	virtual						~CalendarMenuWindow();

	virtual void				Show();
	virtual void				WindowActivated(bool active);
	virtual void				MessageReceived(BMessage* message);

private:
			void				_UpdateDate(const BDate& date);
			BButton*			_SetupButton(const char* label, uint32 what,
									float height);

private:
			BStringView*		fYearLabel;
			BStringView*		fMonthLabel;
			BCalendarView*		fCalendarView;
			bool				fSuppressFirstClose;
};


#endif	/* _CALENDAR_MENU_WINDOW_H_ */
