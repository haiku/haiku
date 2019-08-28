#ifndef _SERIAL_MOUSE_
#define _SERIAL_MOUSE_

#include <keyboard_mouse_driver.h>

enum mouse_id {
	kNoDevice		= -2,
	kUnknown		= -1,	// Something there, but can't recognize it yet.
	kNotSet			=  0,
	kMicrosoft,				// 3-bytes, 2 and 3 buttons.
	kLogitech,				// (3+1)-bytes, 3-buttons.
	kMouseSystems,			// 5-bytes, 3-buttons.
	kIntelliMouse			// 4-bytes, up to 5 buttons, 1 or 2 wheels.
};

class BSerialPort;
class SerialMouse {
public:
				SerialMouse();
	virtual		~SerialMouse();

	status_t	IsMousePresent();
	mouse_id	MouseID() { return fMouseID; };
	const char*	MouseDescription();

	status_t	GetMouseEvent(mouse_movement* mm);

private:
	mouse_id	DetectMouse();
	mouse_id	ParseID(char buffer[], uint8 length);
	status_t	SetPortOptions();

	status_t	GetPacket(char data[]);
	status_t	PacketToMM(char data[], mouse_movement* mm);

	void		DumpData(mouse_movement* mm);

	BSerialPort*	fSerialPort;
	uint8			fPortsCount;
	uint8			fPortNumber;	// The port in use.
	mouse_id		fMouseID;

	uint8		fButtonsState;
};

#endif // _SERIAL_MOUSE_
