#ifndef REFRESHSLIDER_H
#define REFRESHSLIDER_H

#include <Slider.h>

class RefreshSlider : public BSlider
{

public:
					RefreshSlider(BRect frame);
					~RefreshSlider();
	virtual void 	DrawFocusMark();
	virtual char* 	UpdateText() const;
	virtual void	KeyDown(const char *bytes, int32 numBytes);
	
private:
	char*			fStatus;
};

#endif