#ifndef TIME_WINDOW_H
#define TIME_WINDOW_H

#include <Window.h>

#include "TimeSettings.h"
#include "TimeView.h"

class TimeWindow : public BWindow 
{
public:
	TimeWindow();
	~TimeWindow();
	
	bool QuitRequested();
	void MessageReceived(BMessage *message);
	void BuildView();
	
private:
	TimeView	*fView;

};

#endif
