#ifndef KEYBOARD_SETTINGS_H_
#define KEYBOARD_SETTINGS_H_

#include <SupportDefs.h>

typedef struct {
        bigtime_t       key_repeat_delay;
        int32           key_repeat_rate;
} kb_settings;

class KeyboardSettings{
public :
	KeyboardSettings();
	~KeyboardSettings();
	
	BPoint WindowCorner() const { return fCorner; }
	void SetWindowCorner(BPoint corner);
	int32 KeyboardRepeatRate() const { return fSettings.key_repeat_rate; }
	void SetKeyboardRepeatRate(int32 rate);
	int32 KeyboardRepeatDelay() const { return fSettings.key_repeat_delay; }
	void SetKeyboardRepeatDelay(int32 rate);
	
private:
	static const char 	kKeyboardSettingsFile[];
	BPoint				fCorner;
	kb_settings			fSettings;
};

#endif