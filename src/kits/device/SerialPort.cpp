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

/* The directory where the serial driver publishes its devices */ 
#define SERIAL_DIR "/dev/ports" 


/* Creates and initializes a BSerialPort object,	*/
/* query the driver, and builds a list of available */
/* serial ports.									*/

/* The BSerialPort object is initialized to these   */
/* values: */
/* 19200 BPS,	*/
/* 8 Data Bits, */
/* 1 Stop Bit,	*/
/* No Parity,	*/
/* Hardware Flow Control, */
/* Infinite Timeout	*/
/* and Blocking mode. */	
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


/* Closes the port, if it's open, and deletes the devices list */ 
BSerialPort::~BSerialPort()
{
	if (ffd > 0)
		close(ffd);
	
	for (int32 count = _fDevices->CountItems() - 1; count >= 0; count--)
		free(_fDevices->RemoveItem(count));
	
	delete _fDevices;
}


/* Opens a serial port. @param a valid port name */
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


/* Closes the port */
void
BSerialPort::Close(void)
{
	if (ffd > 0)
		close(ffd);
	ffd = -1;
}


/* Read some data from the serial port. */
/* @param the buffer where to transfer the data */
/* @param how many bytes to read	*/
ssize_t
BSerialPort::Read(void *buf, size_t count)
{
	if (ffd < 0) // We have no open port
		return B_FILE_ERROR; 
	
	return read(ffd, buf, count);
}


/* Write some data to the serial port. */
/* @param the buffer from which transfer the data */
/* @param how many bytes to write	*/
ssize_t
BSerialPort::Write(const void *buf, size_t count)
{
	if (ffd < 0) // We have no open port
		return B_FILE_ERROR;

	return write(ffd, buf, count);
}


/* Set blocking mode */
void
BSerialPort::SetBlocking(bool Blocking)
{
	fBlocking = Blocking;
	DriverControl();
}


/* Set the timeout for the port */
/* Valid values: B_INFINITE_TIMEOUT or any value between 0 and 25000000 */
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


/* Set the data rate (Baud rate) for the port */
status_t
BSerialPort::SetDataRate(data_rate bitsPerSecond)
{
	fBaudRate = bitsPerSecond;
	
	return DriverControl();
}


/* Get the data rate (Baud Rate) */
data_rate
BSerialPort::DataRate(void)
{
	return fBaudRate;
}


/* Set the data bits (7 or 8) */
void
BSerialPort::SetDataBits(data_bits numBits)
{
	fDataBits = numBits;
	DriverControl();
}


/* Get the data bits */
data_bits
BSerialPort::DataBits(void)
{
	return fDataBits;
}


/* Set the stop bits (1 or 2) */
void
BSerialPort::SetStopBits(stop_bits numBits)
{
	fStopBits = numBits;
	DriverControl();
}


/* Get the stop bits */
stop_bits
BSerialPort::StopBits(void)
{
	return fStopBits;
}


/* Set the parity mode (ODD, PAIR, or NONE) */
void
BSerialPort::SetParityMode(parity_mode which)
{
	fParityMode = which;
	DriverControl();
}


/* Get the parity mode */
parity_mode
BSerialPort::ParityMode(void)
{
	return fParityMode;
}


/* Clear the input buffer */
void
BSerialPort::ClearInput(void)
{
	tcflush(ffd, TCIFLUSH);
}


/* Clear the output buffer */
void
BSerialPort::ClearOutput(void)
{
	tcflush(ffd, TCOFLUSH);
}


/* Set the flow control (HARDWARE, SOFTWARE, or NONE) */ 
void
BSerialPort::SetFlowControl(uint32 method)
{
	fFlow = method;
	DriverControl();
}


/* Get the flow control */
uint32
BSerialPort::FlowControl(void)
{
	return fFlow;
}


/* Set the DTR */
status_t
BSerialPort::SetDTR(bool asserted)
{
	status_t status = ioctl(ffd, TCSETDTR, &asserted);
	
	return (status > 0) ? status : errno;
}


/* Set the RTS status */
status_t
BSerialPort::SetRTS(bool asserted)
{
	status_t status = ioctl(ffd, TCSETRTS, &asserted);
	
	return (status > 0) ? status : errno;
}


/* See how many chars are queued on the serial port, */
/* waiting to be read */
status_t
BSerialPort::NumCharsAvailable(int32 *wait_until_this_many)
{
	//No help from the BeBook...
	if (ffd < 0)
		return B_ERROR;
	
	//TODO: Implement
	return B_OK;
}


/* See if CTS is set */
bool
BSerialPort::IsCTS(void)
{
	unsigned int bits = ioctl(ffd, TCGETBITS, 0); 
	
	if (bits & TCGB_CTS)
		return true;
		
	return false;
}


/* See if DSR is set */
bool
BSerialPort::IsDSR(void)
{
	unsigned int bits = ioctl(ffd, TCGETBITS, 0); 
	
	if (bits & TCGB_DSR)
		return true;
		
	return false;
}


/* See if RI is set */
bool
BSerialPort::IsRI(void)
{
	unsigned int bits = ioctl(ffd, TCGETBITS, 0); 
	
	if (bits & TCGB_RI)
		return true;
		
	return false;
}


/* See if DCD is set */
bool
BSerialPort::IsDCD(void)
{
	unsigned int bits = ioctl(ffd, TCGETBITS, 0); 
	
	if (bits & TCGB_DCD)
		return true;
		
	return false;
}


/* Wait until there's something to read from the serial port. */
/* If no data is ready, it will always block, ignoring the    */
/* value of SetBlocking(); however, it respects the timeout   */
/* set by SetTimeout(). */
ssize_t
BSerialPort::WaitForInput(void)
{
	ssize_t size;
	int err = ioctl(ffd, TCWAITEVENT, &size, sizeof size);
	
	return (err < B_OK) ? errno : size;
}


/* Returns the number of available Serial Ports. */
int32
BSerialPort::CountDevices()
{
	int32 count = 0;
	
	if (_fDevices != NULL)
		count = _fDevices->CountItems();
	
	return count;
}


/* Get the device name for the given device.*/
/* The first parameter is the number of the device */
/* you want to know the name, the second is the buffer */
/* where you want to store the name, and the third is  */
/* the length of that buffer. */
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


/* Private or Reserved */

/* Query the serial driver about the available devices, */
/* and build a list of them. */
void
BSerialPort::ScanDevices()
{
	BEntry entry;
	BDirectory dir(SERIAL_DIR);
	char buf[B_OS_NAME_LENGTH];
	
	//First, we empty the list
	for (int32 count = _fDevices->CountItems() - 1; count >= 0; count--)
		free(_fDevices->RemoveItem(count));
		
	//Then, add the devices to the list
	while (dir.GetNextEntry(&entry) == B_OK)
	{
		entry.GetName(buf);
		_fDevices->AddItem(strdup(buf));
	};
}


/* Send the options to the serial driver. */
/* Returns B_OK if all goes fine, an error code */
/* if something goes wrong. */
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


/* These functions are here to maintain Binary Compatibility */ 
void BSerialPort::_ReservedSerialPort1() {}
void BSerialPort::_ReservedSerialPort2() {}
void BSerialPort::_ReservedSerialPort3() {}
void BSerialPort::_ReservedSerialPort4() {}
