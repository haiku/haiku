#ifndef POS_SETTINGS_H_
#define POS_SETTINGS_H_

#include <SupportDefs.h>
#include <Screen.h>

class PosSettings{
public :
	PosSettings();
	virtual ~PosSettings();
	BRect WindowPosition() const { return fWindowFrame; }
	void SetWindowPosition(BRect);
	
private:
	static const char 	kVMSettingsFile[];
	BRect				fWindowFrame;
	BPoint				fcorner;
	BPoint				brCorner;
};

#endif