/*
 * Copyright 2002, Jack Burton.
 * Copyright 2002, Marcus Overhagen.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Directory.h>
#include <Entry.h>
#include <List.h>
#include <SerialPort.h>

#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define SERIAL_DIR "/dev/ports"

BSerialPort::BSerialPort()
	:	ffd(-1),
		fBaudRate(B_19200_BPS),
		fDataBits(B_DATA_BITS_8),
		fStopBits(B_STOP_BIT_1),
		fParityMode(B_NO_PARITY),
		fFlow(B_HARDWARE_CONTROL),
		fTimeout(B_INFINITE_TIMEOUT),
		fBlocking(true),
		_fDevices(new BList)
{
	ScanDevices();
}


BSerialPort::~BSerialPort()
{
	if (ffd > 0)
		close(ffd);
	
	for (int32 count = _fDevices->CountItems() - 1; count >= 0; count--)
		free(_fDevices->ItemAt(count));
	
	delete _fDevices;
}


status_t
BSerialPort::Open(const char *portName)
{
	char buf[64];
	
	//TODO: Check if portName is a valid name
	sprintf(buf, SERIAL_DIR"/%s", portName);
	
	if (ffd > 0) //If this port is already open, close it
		close(ffd);
	
	ffd = open(buf, O_RDWR|O_NONBLOCK); //R5 seem to use this mask 
	
	if (ffd > 0)
		DriverControl(); //Setup the port
	
	return (ffd < 0) ? ffd : B_OK;
}


void
BSerialPort::Close(void)
{
	if (ffd > 0)
		close(ffd);
	ffd = -1;
}


ssize_t
BSerialPort::Read(void *buf, size_t count)
{
	if (ffd < 0) // We have no open port
		return B_FILE_ERROR; 
	
	return read(ffd, buf, count);
}


ssize_t
BSerialPort::Write(const void *buf, size_t count)
{
	if (ffd < 0) // We have no open port
		return B_FILE_ERROR;

	return write(ffd, buf, count);
}


void
BSerialPort::SetBlocking(bool Blocking)
{
	fBlocking = Blocking;
	DriverControl();
}


status_t
BSerialPort::SetTimeout(bigtime_t microSeconds)
{	
	status_t err = B_BAD_VALUE;
	
	if (microSeconds == B_INFINITE_TIMEOUT || microSeconds <= 25000000)
	{
		fTimeout = microSeconds;
		DriverControl();
		err = B_OK;
	}
	return err;
}


status_t
BSerialPort::SetDataRate(data_rate bitsPerSecond)
{
	fBaudRate = bitsPerSecond;
	
	return DriverControl();
}


data_rate
BSerialPort::DataRate(void)
{
	return fBaudRate;
}


void
BSerialPort::SetDataBits(data_bits numBits)
{
	fDataBits = numBits;
	DriverControl();
}


data_bits
BSerialPort::DataBits(void)
{
	return fDataBits;
}


void
BSerialPort::SetStopBits(stop_bits numBits)
{
	fStopBits = numBits;
	DriverControl();
}


stop_bits
BSerialPort::StopBits(void)
{
	return fStopBits;
}


void
BSerialPort::SetParityMode(parity_mode which)
{
	fParityMode = which;
	DriverControl();
}


parity_mode
BSerialPort::ParityMode(void)
{
	return fParityMode;
}


void
BSerialPort::ClearInput(void)
{
	tcflush(ffd, TCIFLUSH);
}


void
BSerialPort::ClearOutput(void)
{
	tcflush(ffd, TCOFLUSH);
}


void
BSerialPort::SetFlowControl(uint32 method)
{
	fFlow = method;
	DriverControl();
}


uint32
BSerialPort::FlowControl(void)
{
	return fFlow;
}


status_t
BSerialPort::SetDTR(bool asserted)
{
	status_t status = ioctl(ffd, TCSETDTR, &asserted);
	
	return (status > 0) ? status : errno;
}


status_t
BSerialPort::SetRTS(bool asserted)
{
	status_t status = ioctl(ffd, TCSETRTS, &asserted);
	
	return (status > 0) ? status : errno;
}


status_t
BSerialPort::NumCharsAvailable(int32 *wait_until_this_many)
{
	//No help from the BeBook...
	if (ffd < 0)
		return B_ERROR;
	
	return B_OK;
}


bool
BSerialPort::IsCTS(void)
{
	unsigned int bits = ioctl(ffd, TCGETBITS, 0); 
	
	if (bits & TCGB_CTS)
		return true;
		
	return false;
}


bool
BSerialPort::IsDSR(void)
{
	unsigned int bits = ioctl(ffd, TCGETBITS, 0); 
	
	if (bits & TCGB_DSR)
		return true;
		
	return false;
}


bool
BSerialPort::IsRI(void)
{
	unsigned int bits = ioctl(ffd, TCGETBITS, 0); 
	
	if (bits & TCGB_RI)
		return true;
		
	return false;
}


bool
BSerialPort::IsDCD(void)
{
	unsigned int bits = ioctl(ffd, TCGETBITS, 0); 
	
	if (bits & TCGB_DCD)
		return true;
		
	return false;
}


ssize_t
BSerialPort::WaitForInput(void)
{
	int size;
	int err = ioctl(ffd, TCWAITEVENT, &size, sizeof(size));
	
	return (err < B_OK) ? errno : size;
}


int32
BSerialPort::CountDevices()
{
	int32 count = 0;
	
	if (_fDevices != NULL)
		count = _fDevices->CountItems();
	
	return count;
}


status_t
BSerialPort::GetDeviceName(int32 n, char *name, size_t bufSize)
{
	status_t result = B_ERROR;
	char *dev = NULL;
	
	if (_fDevices != NULL)
		dev = static_cast<char*>(_fDevices->ItemAt(n));

	if (dev != NULL && name != NULL) {
		strncpy(name, dev, bufSize);
		result = B_OK;
	}
	return result;
}


// Private or Reserved

void
BSerialPort::ScanDevices()
{
	BEntry entry;
	BDirectory dir(SERIAL_DIR);
	char buf[B_OS_NAME_LENGTH];
	
	//First, we empty the list
	for (int32 count = _fDevices->CountItems() - 1; count >= 0; count--)
		free(_fDevices->ItemAt(count));
	
	_fDevices->MakeEmpty();
	
	//Then, add the devices to the list
	while (dir.GetNextEntry(&entry) == B_OK)
	{
		entry.GetName(buf);
		_fDevices->AddItem(strdup(buf));
	};
}


int
BSerialPort::DriverControl()
{
	struct termio termioControl;
	int err;
	
	if (ffd < 0)
		return B_NO_INIT;
	
	//Load the current settings	
	err = ioctl(ffd, TCGETA, &termioControl);
	if (err < 0)
		return errno;
		
	// Reset all flags	
	termioControl.c_cflag &= ~(CRTSCTS | CSIZE | CBAUD | CSTOPB | PARODD | PARENB);
	termioControl.c_iflag &= ~(IXON | IXOFF | IXANY | INPCK);
	termioControl.c_lflag &= ~(ECHO | ECHONL | ISIG | ICANON);
		
	//Set the flags to the wanted values
	if (fFlow & B_HARDWARE_CONTROL)
		termioControl.c_cflag |= CRTSCTS;
	
	if (fFlow & B_SOFTWARE_CONTROL)
		termioControl.c_iflag |= (IXON | IXOFF);
	
	if (fStopBits & B_STOP_BITS_2)
		termioControl.c_cflag |= CSTOPB; // We want 2 stop bits
	
	if (fDataBits == B_DATA_BITS_8)
		termioControl.c_cflag |= CS8; // We want 8 data bits
	
	//Ok, set the parity now
	if (fParityMode != B_NO_PARITY)
	{
		termioControl.c_cflag |= PARENB; //Enable parity
		if (fParityMode == B_ODD_PARITY)
			termioControl.c_cflag |= PARODD; //Select odd parity
	}
	
	//Set the baud rate	
	termioControl.c_cflag |= (fBaudRate & CBAUD); 
	
	//Set the timeout
	if (fBlocking)
	{
		if (fTimeout == B_INFINITE_TIMEOUT)
		{
			termioControl.c_cc[VTIME] = 0;
			termioControl.c_cc[VMIN] = 1;
		} 
		else if (fTimeout == 0)
			termioControl.c_cc[VMIN] = 0;
		else 
		{
			int t = fTimeout / 100000;
			termioControl.c_cc[VTIME] = (t == 0) ? 1 : t;
			termioControl.c_cc[VMIN] = 1;
		}
	} else
		termioControl.c_cc[VMIN] = 0;
		
	//Ok, finished. Now tell the driver what we decided	
	err = ioctl(ffd, TCSETA, &termioControl);
	
	return err > 0 ? err : errno;	
}


//FBC
void BSerialPort::_ReservedSerialPort1() {}
void BSerialPort::_ReservedSerialPort2() {}
void BSerialPort::_ReservedSerialPort3() {}
void BSerialPort::_ReservedSerialPort4() {}
