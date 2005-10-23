//------------------------------------------------------------------------------
//	Copyright (c) 2004, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		SerialMouse.cpp
//	Author(s):		Oscar Lesta (bipolar@softhome.net)					
//	Description:	SerialMouse detects and manages serial mice, duh!.
//  References:		- http://www.hut.fi/~then/mytexts/mouse.html
//					- Be's (binary) serial_mouse addon.
//					- Haiku's CVS.
//------------------------------------------------------------------------------

#include <string.h>

#include <SerialPort.h>

#include "SerialMouse.h"

//#define DEBUG_SERIAL_MOUSE
#ifdef DEBUG_SERIAL_MOUSE
	#include <stdio.h>
	#define LOG(x)		printf x	// TODO: log to "MouseInputDevice::sLogFile"
									// I used this in my console tests.
#else
	#define LOG(x)
#endif

const static bigtime_t kSerialTimeOut = 200000;	// 200 ms
const static uint8 kMaxBytesToRead = 255;	// Serial PnP data can be this long.

// The protocols we know how to handle. Indexed by mouse_protocol_id.
/*
  the sync[] is a protocol-identification/sync thingy:

  if ((read_byte[0] & sync[0]) == sync[1]) then we are at the beggining of the
  a packet. Next data bytes are OK... if ((read_byte[i] & sync[2]) == 0))
*/
struct mouse_protocol {
	const char* name;
	uint8 num_bytes;
	uint8 sync[3];
};


const static
struct mouse_protocol mp[] = {
	{ "UNKNOWN",       0, { 0x00, 0x00, 0x00 } },
	{ "Microsoft",     3, { 0x40, 0x40, 0x40,} },
	{ "Logitech",      3, { 0x40, 0x40, 0x40 } }, // 3/4 bytes. FIX: only 3 are used now.
	{ "MouseSystems",  5, { 0xF8, 0x80, 0x00 } },
	{ "IntelliMouse",  4, { 0x40, 0x40, 0x00 } },
};


SerialMouse::SerialMouse()
	: fSerialPort(NULL),
	fPortsCount(0),
	fPortNumber(0),
	fMouseID(kNotSet),
	fButtonsState(0)
{
	fSerialPort = new BSerialPort();
	fPortsCount = fSerialPort->CountDevices() - 1;
}


SerialMouse::~SerialMouse()
{
	if (fSerialPort != NULL) {
		fSerialPort->SetRTS(false);		// Put the mouse to sleep.
		fSerialPort->Close();
		delete fSerialPort;
	}
}

//	#pragma mark -

const char *
SerialMouse::MouseDescription()
{
	// TODO: If we'll support more than just one mouse, this should be changed.
	// Maybe we should also add the port number as suffix.
	
	return mp[fMouseID].name;
}


// Find first usable serial port, try to detect a mouse there.
// Returns:
// - B_NO_INIT: all available ports were tested (no mouse present there).
// - A positive value indicating in which serial port a mouse was found.

status_t
SerialMouse::IsMousePresent()
{
	for (uint8 i = 1; i <= fPortsCount; i++) {
		char dev_name[B_PATH_NAME_LENGTH];

		fSerialPort->GetDeviceName(i, dev_name);

		// Skip internal modem (pctel, lucent or trimodem drivers).
		// previously I checked only for != "pctel", now for == "serial#"
		if (strncmp(dev_name, "serial", 5) != 0)
			continue;

		if (fSerialPort->Open(dev_name) <= 0) {
			LOG(("SerialMouse: Failed to open %s.\n", dev_name));
			continue;	// try next port.
		}

		LOG(("SerialMouse : Opened %s.\n", dev_name));

		// This 4x retries helps, a little, to detect one of my mouse (buggy?).
		// Not perfect, but catchs it more often than before.
		uint8 retries = 4;
		do {
			fMouseID = DetectMouse();
			if (fMouseID > kNotSet) {
				// We found a mouse, just break the loop.
				LOG(("SerialMouse: Found a %s Mouse.\n", MouseDescription()));
				fSerialPort->SetBlocking(true);
				fPortNumber = i;
				return fPortNumber; 		// Return the port # in use.
			}
		} while ((fMouseID == kUnknown) && (retries--));

		LOG(("SerialMouse: Mouse not detected.\n"));
		fSerialPort->Close();
	}

	// If we get here its because we didn't found a mouse in any of the
	// available serial ports.
	// (Caller can do: "while (sm->IsMousePresent() > 0)").

	return B_NO_INIT;
}


mouse_id
SerialMouse::DetectMouse()
{
	int bytes_read = 0;
	char c;
	char id_buffer[20];			// If the mouse sends an ID, we store it here.
	char buffer[kMaxBytesToRead];
	uint8 id_length = 0;

	fSerialPort->SetBlocking(false);
	fSerialPort->SetTimeout(kSerialTimeOut);

	SetPortOptions();

	snooze(10000);

	// Toggle RTS line in order to 'wake up' the mouse
	fSerialPort->SetRTS(false);
	snooze(120000);				// RTS low pulse width must be at least 100ms.
	fSerialPort->ClearInput();
	fSerialPort->SetRTS(true);

	// wait upto kSerialTimeOut ms while trying to read mouse ID string.
	if (fSerialPort->WaitForInput() == 0)
		return kNoDevice;			// nothing there, quit.

	// we make sure to 'eat' everything the mouse sends at init time, albeit
	// we are interested only on the few first bytes, not doing it so will
	// confuse things later.
	while ((fSerialPort->Read(&c, 1) == 1) && (bytes_read < kMaxBytesToRead)) {
		LOG(("read = %c (%d d - %x h)\n", c, c, c));

		// Collect the bytes we care about.
		if (c == 'M' || c == 'H' || c == '3' || c == 'Z' || c == '@') {
			if (id_length < 4) {
				id_buffer[id_length] = c;
				id_length++;
			}
		} else 
			buffer[bytes_read] = c;	// store the garbage for futher processing.

		bytes_read++;

		// is there something else waiting to be read?
		if (fSerialPort->WaitForInput() == 0)
			break;								// no, break the loop.
	}

	// This can't happen, but... if we didn't get any data, just quit.
	if (bytes_read == 0)
		return kNoDevice;

	fSerialPort->ClearInput();	// Toldya, I have a very noisy mouse.

	if (id_length) {
		fMouseID = ParseID(id_buffer, id_length);
		SetPortOptions();				// Set new options according to MouseID.
		return fMouseID;
	}

	// TODO: Below this line is work in progress (ie. temporal hacks until I,
	// or someone else, get something better).

	// Ok, last resort... try to identify the beast according to its packets.

	if (bytes_read < 3)		// not enough data to even start... quit.
		return kUnknown;

	// First attempt to identify a MouseSystems mouse, because some (most?) of
	// them either send: nothing, an standard packet or just garbage.
	if (bytes_read == 5) {
		// TODO: validate the packet!
		fMouseID = kMouseSystems;
		SetPortOptions();
		return fMouseID;
	}

	return kUnknown;
}


// See if we recognize the mouse ID (first bytes mice usually send after
// toggling the DTR/RTS lines on the serial port). Usual values:
//
// Microsoft mode    = 'M'
// Microsoft mode    = 'M3'
// IntelliMouse mode = 'MZ' and a valid 4-bytes null packet (0x40,0,0,0)
// MouseSystems mode = 'HH', nothing at all, 5-bytes packet, or just garbage.

mouse_id
SerialMouse::ParseID(char buffer[], uint8 length)
{
	LOG(("data length = $d\n", length));

	if ((length == 1) && (buffer[0] == 'M'))
		return kMicrosoft;

	if (length == 2) {
		if (buffer[0] == 'M' && buffer[1] == '3')
			return kLogitech;
		else if (buffer[0] == 'H' && buffer[1] == 'H')
			return kMouseSystems;
	}

	if ((length == 4) &&
		(buffer[0] == 'M' && buffer[1] == 'Z') && (buffer[2] == '@'))
		return kIntelliMouse;

	return kUnknown;
}


// Set serial port options according to our different needs.
status_t
SerialMouse::SetPortOptions()
{
	switch (fMouseID) {
		case kLogitech:
			fSerialPort->SetDataRate(B_1200_BPS);
			fSerialPort->Write("*q", 2);
			fSerialPort->SetDataRate(B_9600_BPS);
			fSerialPort->SetDataBits(B_DATA_BITS_7);
			break;

		case kMouseSystems:
			fSerialPort->SetDataRate(B_1200_BPS);
			fSerialPort->SetDataBits(B_DATA_BITS_8);
			break;

		case kNotSet:
		default:
			// other defaults values are ok for us: no parity, 1 stop bit.
			fSerialPort->SetDataRate(B_1200_BPS);
			fSerialPort->SetDataBits(B_DATA_BITS_7);
			break;
	}

	return B_OK;
}


//	#pragma mark -
status_t
SerialMouse::GetMouseEvent(mouse_movement* mm)
{
	char data[5];

	if (fMouseID <= kNotSet)
		return B_ERROR;

	if (GetPacket(data) != B_OK)
		return B_ERROR;		// not enough, or out-of-sync, data.

	if (PacketToMM(data, mm) != B_OK)
		return B_ERROR;     // something went wrong

#ifdef DEBUG_SERIAL_MOUSE
	DumpData(mm);
#endif

	return B_OK;
}


//	#pragma mark -

// Block until we read enough bytes for the current protocol. See if we're in
// sync with data stream, if not, just skip data until we regain sync.

// TODO: syncronization needs a re-write. We should keep track of what byte we
// are currently working on, validate it against mice's protocol, and if valid,
// increment a "current_packet_byte" counter.
// Also: I'm currently skipping the optional 4th byte in the Logitech protocol.
// (may work, but 3th button will not).


status_t
SerialMouse::GetPacket(char data[])
{
	status_t result = B_ERROR;
	uchar c = 0;
	uint8    bytes_read;
	size_t   mpsize = mp[fMouseID].num_bytes;

	for (bytes_read = 0; bytes_read < mpsize; bytes_read++) {
		// TODO: Shall we block here instead of leaving after a timeout?
		// Yes, if we get called it's because there IS a mouse out there.

		if (fSerialPort->Read(&c, 1) != 1) {
			snooze(5000); // this is a realtime thread, and something is wrong...
			break;
		}

		if (bytes_read == 0) {
			if ((c & mp[fMouseID].sync[0]) != mp[fMouseID].sync[1]) {
				LOG(("Out of sync: skipping byte = %x\n", c));
				continue;	// skip bytes until we get a "header" byte.
			}
		}
		data[bytes_read] = c;
	}

	if (bytes_read == mpsize) {
		result = B_OK;

		// validate data...
		for (uint8 i = 1; i <= mpsize; i++) {
			if ((data[i] & mp[fMouseID].sync[2]) != 0) {
				LOG(("Out of sync: wrong data byte = %x\n", data[i]));
				result = B_ERROR;
				break;	// skip the packet.
			}
		}
	}

	return result;
}


// kMicroSoft, kMouseSystem, kIntelliMouse: working OK.
// kLogitech: untested by lack of such devices. 3th button probably won't work.
status_t
SerialMouse::PacketToMM(char data[], mouse_movement* mm)
{
	const uint8 kPrimaryButton   = 1;
	const uint8 kSecondaryButton = 2;
	const uint8 kTertiaryButton  = 4;

	static uint8 previous_buttons = 0;	// only meaningful for kMicrosoft.
	
	mm->timestamp = system_time();

	switch (fMouseID) {
		case kMicrosoft:
			mm->buttons = ((data[0] & 0x20) ? kPrimaryButton   : 0) +
						  ((data[0] & 0x10) ? kSecondaryButton : 0);

			//                        Higher 2 bits          Lower 6 bits
			mm->xdelta =   (int8) (((data[0] & 0x03) << 6) + (data[1] & 0x3F));
			mm->ydelta = - (int8) (((data[0] & 0x0C) << 4) + (data[2] & 0x3F));

			// up to here we've handled "ye olde" 2-buttons MS packet.
			// There's a 3-buttons extension to it, consisting in sending a
			// "null packet" (no changes in x/y nor in standard buttons)
			// whenever the third button is pressed/released (how clever!! :-P).

			if ((mm->xdelta == 0) && (mm->ydelta == 0) &&
				((uint8) mm->buttons == (previous_buttons & ~kTertiaryButton))) {
				// no movement, nor button change: toggle middle
				mm->buttons = previous_buttons ^ kTertiaryButton;
			} else {
				// change: preserve middle
				mm->buttons |= previous_buttons & kTertiaryButton;
			}

			previous_buttons = mm->buttons;
			break;

		case kIntelliMouse:
			// TODO: add support for 4th and 5th buttons?
			mm->buttons =	((data[0] & 0x20) ? kPrimaryButton   : 0) +
							((data[0] & 0x10) ? kSecondaryButton : 0) +
							((data[3] & 0x10) ? kTertiaryButton  : 0);

			mm->xdelta =   ((int8) ((data[0] & 0x03) << 6) + (int8) (data[1] & 0x3F));
			mm->ydelta = - ((int8) ((data[0] & 0x0C) << 4) + (int8) (data[2] & 0x3F));

			switch (data[3] & 0x0F) {
				case 0x1: mm->wheel_ydelta = +1; break; // wheel 1 down
				case 0xF: mm->wheel_ydelta = -1; break; // wheel 1 up
				case 0x2: mm->wheel_xdelta = +1; break; // wheel 2 down
				case 0xE: mm->wheel_xdelta = -1; break; // wheel 2 up
			}
			break;

		case kMouseSystems:
		{
			uint8 tmp = (~data[0] & 0x07);

			mm->buttons =	((tmp & 0x4) ? kPrimaryButton   : 0) +
							((tmp & 0x1) ? kSecondaryButton : 0) +
							((tmp & 0x2) ? kTertiaryButton  : 0);

			mm->xdelta = ((int8) data[1] + (int8) data[3]);
			mm->ydelta = ((int8) data[2] + (int8) data[4]);
			break;
		}
		
		case kLogitech:
		{
//			uint8 tmp = (data[3] & 0x20);	// 3th button bit.

			mm->buttons = ((data[0] & 0x20) ? kPrimaryButton   : 0) +
						  ((data[0] & 0x10) ? kSecondaryButton : 0);
//						 + ((tmp)            ? kTertiaryButton  : 0);

			//                        Higher 2 bits          Lower 6 bits
			mm->xdelta =   (int8) (((data[0] & 0x03) << 6) + (data[1] & 0x3F));
			mm->ydelta = - (int8) (((data[0] & 0x0C) << 4) + (data[2] & 0x3F));
			break;
		}
		
		default:
			LOG(("Unhandled protocol. Should not happen.\n"));
			return B_ERROR;
	}

	return B_OK;
}


//	#pragma mark -


void
SerialMouse::DumpData(mouse_movement* mm)
{
#ifdef DEBUG_SERIAL_MOUSE
	if (mm->buttons ^ fButtonsState)
		LOG(("Buttons = %ld\n", mm->buttons));

	if (mm->xdelta || mm->ydelta)
		LOG(("xdelta = %ld; ydelta = %ld\n", mm->xdelta, mm->ydelta));

	if (fMouseID == kIntelliMouse && (mm->wheel_xdelta || mm->wheel_xdelta)) {
		LOG(("wheel_xdelta = %ld; wheel_ydelta = %ld\n", mm->wheel_xdelta,
			mm->wheel_ydelta));
	}
#endif
}
