#ifndef SCREENSETTINGS_H
#define SCREENSETTINGS_H

#include <View.h>

class ScreenSettings
{

public:
						ScreenSettings();
	virtual 			~ScreenSettings();

	BRect 				WindowFrame() const { return fWindowFrame; };
	void 				SetWindowFrame(BRect);

private:
	static const char 	fScreenSettingsFile[];
	BRect 				fWindowFrame;
};

#endif