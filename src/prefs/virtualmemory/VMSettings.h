#ifndef VM_SETTINGS_H_
#define VM_SETTINGS_H_

#include <SupportDefs.h>
#include <Screen.h>

class VMSettings{
public :
	VMSettings();
	virtual ~VMSettings();
	BRect WindowPosition() const { return fWindowFrame; }
	void SetWindowPosition(BRect);
	
private:
	static const char 	kVMSettingsFile[];
	BRect				fWindowFrame;
	BPoint				fcorner;
};

#endif