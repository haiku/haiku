#ifndef CONFIG_VIEW
#define CONFIG_VIEW
/* ConfigView - the configuration view for the Folder filter
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <View.h>


class ConfigView : public BView
{
	public:
		ConfigView();
		void SetTo(BMessage *archive);

		virtual	status_t Archive(BMessage *into, bool deep = true) const;
		virtual	void GetPreferredSize(float *width, float *height);
};

#endif	/* CONFIG_VIEW */
