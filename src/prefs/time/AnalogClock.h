#ifndef ANALOG_CLOCK_H
#define ANALOG_CLOCK_H

#include <Message.h>
#include <View.h>

class TOffscreen: public BView {
	public:
		TOffscreen(BRect frame, const char *name);
		virtual ~TOffscreen();

		virtual void DrawX();
		BPoint Position();
	
		BPoint f_MinutePoints[60];
		BPoint f_HourPoints[60];
		short f_Hours;
		short f_Minutes;
		short f_Seconds;
	private:
		BBitmap *f_bitmap;
		BBitmap *f_centerbmp;
		BBitmap *f_capbmp;
		BPoint f_center;
};

class TAnalogClock: public BView {
	public:
		TAnalogClock(BRect frame, const char *name, uint32 resizingmode, uint32 flags);
		virtual ~TAnalogClock();
		
		virtual void AttachedToWindow();
		virtual void Draw(BRect updaterect);		
		virtual void MessageReceived(BMessage *);
		
		void InitView(BRect frame);
		void SetTo(int32, int32, int32);
	private:
		BPoint f_drawpt;
		BBitmap *f_bitmap;
		TOffscreen *f_offscreen;
};

#endif //ANALOG_CLOCK_H
