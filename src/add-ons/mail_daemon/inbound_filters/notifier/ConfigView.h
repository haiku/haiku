#ifndef CONFIG_VIEW
#define CONFIG_VIEW
/* ConfigView - the configuration view for the Notifier filter
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <View.h>


enum {
	do_beep = 1,
	alert = 2,
	blink_leds = 4,
	big_doozy_alert = 8,
	one_central_beep = 16,
	log_window = 32
};

class ConfigView : public BView
{
	public:
		ConfigView();
		void SetTo(const BMessage *archive);
		virtual	status_t Archive(BMessage *into, bool deep = true) const;

		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *msg);
		virtual	void GetPreferredSize(float *width, float *height);

		void UpdateNotifyText();
};

#endif	/* CONFIG_VIEW */
