#include "usb_p.h"


Object::Object(BusManager *bus)
{
	fStack = bus->GetStack();
	fUSBID = fStack->GetUSBID(this);
}
