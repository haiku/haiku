#ifndef MOUSE_SETTINGS_H_
#define MOUSE_SETTINGS_H_

#include <SupportDefs.h>
#include <InterfaceDefs.h>

typedef enum {
        MOUSE_1_BUTTON = 1,
        MOUSE_2_BUTTON,
        MOUSE_3_BUTTON
} mouse_type;


typedef struct {
        bool    enabled;        // Acceleration on / off
        int32   accel_factor;   // accel factor: 256 = step by 1, 128 = step by 1/2
        int32   speed;          // speed accelerator (1=1X, 2 = 2x)...
} mouse_accel;

typedef struct {
        mouse_type      type;
        mouse_map       map;
        mouse_accel     accel;
        bigtime_t       click_speed;
} mouse_settings;

class MouseSettings{
public :
	MouseSettings();
	~MouseSettings();
	
	BPoint WindowCorner() const { return fCorner; }
	void SetWindowCorner(BPoint corner);
	mouse_type MouseType() const { return fSettings.type; }
	void SetMouseType(mouse_type type);
	bigtime_t ClickSpeed() const { return -(fSettings.click_speed-1000000); } // -1000000 to correct the Sliders 0-100000 scale
	void SetClickSpeed(bigtime_t click_speed);
	int32 MouseSpeed() const { return fSettings.accel.speed; }
	void SetMouseSpeed(int32 speed);
	
private:
	static const char 	kMouseSettingsFile[];
	BPoint				fCorner;
	mouse_settings		fSettings;
};

#endif