#ifndef CALENDAR_VIEW_H
#define CALENDAR_VIEW_H

#include <View.h>

#include <time.h>

class TDay: public BView
{
	public:
		TDay(BRect frame, int day);
		virtual ~TDay();
		virtual void Draw(BRect updaterect);
		virtual void AttachedToWindow();
		virtual void MouseDown(BPoint where);
		void SetTo(BRect frame, int day);
		void SetDay(int day, bool selected = false);
	protected:
 		virtual void Stroke3dFrame(BRect frame, rgb_color light, rgb_color dark, bool inset);
	private:
		int f_day;
		bool f_isselected;
};

class TCalendarView: public BView
{
	public:
		TCalendarView(BRect frame, const char *name, uint32 resizingmode, uint32 flags);
		virtual ~TCalendarView();
		virtual void AttachedToWindow();
		virtual void Draw(BRect bounds);
		
		void Update(tm *atm);
	protected:
		virtual void InitView();
	private:
		void InitDates();
		void ClearDays();
		void CalcFlags();
		
		BRect	f_dayrect;
		int 	f_month;
		int		f_day;
		int		f_year;
		int		f_wday;
};

#endif // CALENDAR_VIEW_H
