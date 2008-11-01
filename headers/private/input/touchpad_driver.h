//
// kb_mouse_driver.h
//
#ifndef _TOUCHPAD_DRIVER_H
#define _TOUCHPAD_DRIVER_H

#include <SupportDefs.h>
#include <Drivers.h>

#ifdef __cplusplus
extern "C" {
#endif

#define B_ONE_FINGER	0x01
#define B_TWO_FINGER	0x02
#define B_MULTI_FINGER	0x04
#define B_PEN			0x08

typedef struct {
	uint8		buttons;
	uint32		xPosition;
	uint32		yPosition;
	uint8		zPressure;
	uint8		fingers;
	bool		gesture;
	uint8		fingerWidth;
	// 1 - 4	normal width
	// 5 - 11	very wide finger or palm
	// 12		maximum reportable width; extrem wide contact
} touchpad_movement;


#ifdef __cplusplus
}
#endif

#endif

