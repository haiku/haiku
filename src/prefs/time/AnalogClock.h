#ifndef ANALOG_CLOCK_H
#define ANALOG_CLOCK_H

#include <Message.h>
#include <View.h>

#include <time.h>

class TOffscreen: public BView
{
	public:
		TOffscreen(BRect frame, char *name);
		virtual ~TOffscreen();
		virtual void AttachedToWindow();
		virtual void DrawX();

		BPoint Position();
	
		BPoint	f_MinutePoints[60];
		BPoint  f_HourPoints[60];
		short 	f_Hours;
		short f_Minutes;
		short f_Seconds;
	private:
		BBitmap *f_bitmap;
		BPoint f_center;
		BPoint f_offset;
		float f_Offset;
};

class TAnalogClock: public BView
{
	public:
		TAnalogClock(BRect frame, const char *name, uint32 resizingmode, uint32 flags);
		virtual ~TAnalogClock();
		virtual void AttachedToWindow();
		virtual void Draw(BRect updaterect);		
		
		void Update(tm *atm);
	private:
		BPoint f_drawpt;
		BBitmap *f_bitmap;
		TOffscreen *f_offscreen;
};

#endif //ANALOG_CLOCK_H
