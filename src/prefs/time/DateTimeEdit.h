#ifndef DATETIME_EDIT_H
#define DATETIME_EDIT_H

#include <Control.h>
#include <Message.h>

#define OB_UP_PRESS 'obUP'
#define OB_DOWN_PRESS 'obDN'

enum FieldFocus {
	FOCUS_NONE = 0,
	FOCUS_MONTH,
	FOCUS_DAY,
	FOCUS_YEAR,
	FOCUS_HOUR,
	FOCUS_MINUTE,
	FOCUS_SECOND,
	FOCUS_AMPM
};

class TDateTimeEdit: public BControl {
	public:
		TDateTimeEdit(BRect frame, const char *name);
		virtual ~TDateTimeEdit();
		
		virtual void AttachedToWindow();
		virtual void Draw(BRect updaterect);
		virtual void MouseDown(BPoint where);
		virtual void MessageReceived(BMessage *message);
		
		virtual void UpPress()=0;
		virtual void DownPress()=0;
	protected:
		virtual void DispatchMessage();
		virtual void BuildDispatch(BMessage *message)=0;
		
		virtual void Draw3dFrame(BRect frame, bool inset);
		
		BBitmap *f_up;
		BBitmap *f_down;
		BRect	f_uprect;
		BRect	f_downrect;
		
		FieldFocus f_focus;
		int f_holdvalue;
};

class TTimeEdit: public TDateTimeEdit {
	public:
		TTimeEdit(BRect frame, const char *name);
		virtual ~TTimeEdit();
		
		virtual void Draw(BRect updaterect);
		virtual void MouseDown(BPoint where);
		virtual void KeyDown(const char *bytes, int32 numbytes);
		virtual void UpPress();
		virtual void DownPress();
		
		virtual void SetTo(int32, int32, int32);
	private:
		void BuildDispatch(BMessage *message);
		void SetFocus(bool forward);
		void UpdateFocus();
		
		BRect f_hourrect;
		BRect f_minrect;
		BRect f_secrect;
		BRect f_aprect;
		
		int f_hour;
		int f_minute;
		int f_second;
		bool f_isam;
};

class TDateEdit: public TDateTimeEdit {
	public:
		TDateEdit(BRect frame, const char *name);
		virtual ~TDateEdit();
		
		virtual void Draw(BRect updaterect);
		virtual void MouseDown(BPoint where);
		virtual void KeyDown(const char *bytes, int32 numbytes);
		virtual void UpPress();
		virtual void DownPress();
		
		virtual void SetTo(int32, int32, int32);
	private:
		void BuildDispatch(BMessage *message);
		void SetFocus(bool forward);
		void UpdateFocus(bool = false);
		
		BRect f_monthrect;
		BRect f_dayrect;
		BRect f_yearrect;
		int f_month;
		int f_day;
		int f_year;
};

#endif //DATETIME_EDIT_H
