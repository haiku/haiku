#ifndef DATETIME_EDIT_H
#define DATETIME_EDIT_H

#include <View.h>

#define OB_UP_PRESS 'obUP'
#define OB_DOWN_PRESS 'obDN'

enum FieldFocus
{
	FOCUS_NONE,
	FOCUS_MONTH,
	FOCUS_DAY,
	FOCUS_YEAR,
	FOCUS_HOUR,
	FOCUS_MINUTE,
	FOCUS_SECOND,
	FOCUS_AMPM
};

class TDateTimeEdit: public BView
{
	public:
		TDateTimeEdit(BRect frame, const char *name);
		virtual ~TDateTimeEdit();
		
		virtual void Draw(BRect updaterect);
		virtual void AttachedToWindow();
		virtual void MouseDown(BPoint where);
		virtual void MakeFocus(bool focused);
		
		virtual void UpPress();
		virtual void DownPress();
	protected:
		virtual void Draw3dFrame(BRect frame, bool inset);
		
		BBitmap	*f_up;
		BBitmap *f_down;
		BRect	f_uprect;
		BRect	f_downrect;
		
		FieldFocus f_focus;
};

class TTimeEdit: public TDateTimeEdit
{
	public:
		TTimeEdit(BRect frame, const char *name);
		virtual ~TTimeEdit();
		
		virtual void Draw(BRect updaterect);
		virtual void Update(tm *_tm);
		virtual void MouseDown(BPoint where);
		
		virtual void UpPress();
		virtual void DownPress();
		
		void SetTo(int hour, int minute, int second);
	private:
		BRect f_hourrect;
		BRect f_minrect;
		BRect f_secrect;
		BRect f_aprect;
		
		int f_hour;
		int f_minute;
		int f_second;
		bool f_isam;
};

class TDateEdit: public TDateTimeEdit
{
	public:
		TDateEdit(BRect frame, const char *name);
		virtual ~TDateEdit();
		
		virtual void Draw(BRect updaterect);
		virtual void MouseDown(BPoint where);
		virtual void Update(tm *_tm);
		
		virtual void UpPress();
		virtual void DownPress();
		
		void SetTo(int month, int day, int year);
	private:
		BRect f_monthrect;
		BRect f_dayrect;
		BRect f_yearrect;
		int f_month;
		int f_day;
		int f_year;
};

#endif //DATETIME_EDIT_H
