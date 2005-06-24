#ifndef TTZDISPLAY_H
#define TTZDISPLAY_H

#include <View.h>

class TTZDisplay: public BView
{
	public:
		TTZDisplay(BRect frame, const char *name, uint32 resizing, uint32 flags, 
				const char *label = B_EMPTY_STRING, 
				const char *name = B_EMPTY_STRING, 
				int32 hour = 0, int32 minute = 0);
		~TTZDisplay();
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		virtual void ResizeToPreferred();
		
		virtual void Draw(BRect);

		virtual void SetLabel(const char *label);
		virtual void SetText(const char *text);
		virtual void SetTo(int32 hour, int32 minute);
		
		virtual const char *Label() const;
		virtual const char *Text() const;
		virtual const char *Time() const;
	private:
		BString *f_label;
		BString *f_text;
		BString *f_time;

};

#endif //TTZDISPLAY_H
