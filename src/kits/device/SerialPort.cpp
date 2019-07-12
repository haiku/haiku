/*
 * Copyright 2002-2004, Marcus Overhagen, Stefano Ceccherini.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <List.h>
#include <SerialPort.h>

#include <new>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>


/* The directory where the serial driver publishes its devices */
#define SERIAL_DIR "/dev/ports"

// Scans a directory and adds the entries it founds as strings to the
// given list
static int32
scan_directory(const char *directory, BList *list)
{
	BEntry entry;
	BDirectory dir(SERIAL_DIR);
	char buf[B_OS_NAME_LENGTH];

	ASSERT(list != NULL);
	while (dir.GetNextEntry(&entry) == B_OK) {
		entry.GetName(buf);
		list->AddItem(strdup(buf));
	};

	return list->CountItems();
}


/*! \brief Creates and initializes a BSerialPort object.

	Query the driver, and builds a list of the available
	serial ports.
	The BSerialPort object is initialized to these values:
	- \c B_19200_BPS
	- \c B_DATA_BITS_8
	- \c B_STOP_BIT_1
	- \c B_NO_PARITY
	- \c B_HARDWARE_CONTROL
	- \c B_INFINITE_TIMEOUT
	- Blocking mode
*/
BSerialPort::BSerialPort()
	:
	ffd(-1),
	fBaudRate(B_19200_BPS),
	fDataBits(B_DATA_BITS_8),
	fStopBits(B_STOP_BIT_1),
	fParityMode(B_NO_PARITY),
	fFlow(B_HARDWARE_CONTROL),
	fTimeout(B_INFINITE_TIMEOUT),
	fBlocking(true),
	fDevices(new(std::nothrow) BList)
{
	_ScanDevices();
}


/*! \brief Frees the resources associated with the object.
	Closes the port, if it's open, and deletes the devices list.
*/
BSerialPort::~BSerialPort()
{
	if (ffd >= 0)
		close(ffd);

	if (fDevices != NULL) {
		for (int32 count = fDevices->CountItems() - 1; count >= 0; count--)
			free(fDevices->RemoveItem(count));
		delete fDevices;
	}
}


/*! \brief Opens a serial port.
	\param portName A valid port name
		(i.e."/dev/ports/serial2", "serial2", ...)
	\return
	- A positive number if the serialport has been succesfully opened.
	- An errorcode (negative integer) if not.
*/
status_t
BSerialPort::Open(const char *portName)
{
	char buf[64];

	if (portName == NULL)
		return B_BAD_VALUE; // Heheee, we won't crash

	if (portName[0] != '/')
		snprintf(buf, 64, SERIAL_DIR"/%s", portName);
	else
		// A name like "/dev/ports/serial2" was passed
		snprintf(buf, 64, "%s", portName);

	if (ffd >= 0) //If this port is already open, close it
		close(ffd);

	// TODO: BeOS don't use O_EXCL, and this seems to lead
	// to some issues. I added this flag having read some comments
	// by Marco Nelissen on the annotated BeBook.
	// I think BeOS uses O_RDWR|O_NONBLOCK here.
	ffd = open(buf, O_RDWR | O_NONBLOCK | O_EXCL);

	if (ffd >= 0) {
		// we used open() with O_NONBLOCK flag to let it return immediately,
		// but we want read/write operations to block if needed,
		// so we clear that bit here.
		int flags = fcntl(ffd, F_GETFL);
		fcntl(ffd, F_SETFL, flags & ~O_NONBLOCK);

		_DriverControl();
	}
	// TODO: I wonder why the return type is a status_t,
	// since we (as BeOS does) return the descriptor number for the device...
	return (ffd >= 0) ? ffd : errno;
}


/*! \brief Closes the port.
*/
void
BSerialPort::Close(void)
{
	if (ffd >= 0)
		close(ffd);
	ffd = -1;
}


/*! \brief Reads some bytes from the serial port.
	\param buf The buffer where to copy the data.
	\param count The maximum amount of bytes to read.
	\return The amount of data read.
*/
ssize_t
BSerialPort::Read(void *buf, size_t count)
{
	ssize_t err = read(ffd, buf, count);

	return (err >= 0) ? err : errno;
}


/*! \brief Writes some bytes to the serial port.
	\param buf The buffer which copy the data from.
	\param count The amount of bytes to write.
*/
ssize_t
BSerialPort::Write(const void *buf, size_t count)
{
	ssize_t err = write(ffd, buf, count);

	return (err >= 0) ? err : errno;
}


/*! \brief Set blocking mode
	\param Blocking If true, enables the blocking mode. If false, disables it.
*/
void
BSerialPort::SetBlocking(bool Blocking)
{
	fBlocking = Blocking;
	_DriverControl();
}


/*! \brief Set the timeout for the port.
	\param microSeconds The timeout for the port.
	Valid values are:
	- \c B_INFINITE_TIMEOUT
	- Any value between 0 and 25,000,000, but remember that the granularity
		of the serial driver is 100,000 microseconds.
*/
status_t
BSerialPort::SetTimeout(bigtime_t microSeconds)
{
	status_t err = B_BAD_VALUE;

	if (microSeconds == B_INFINITE_TIMEOUT || microSeconds <= 25000000) {
		fTimeout = microSeconds;
		_DriverControl();
		err = B_OK;
	}
	return err;
}


/*! \brief Set the Baud rate for the port.
	\param bitsPerSeconds The baud rate.
	Valid values:
	- \c B_0_BPS
	- \c B_50_BPS
	- \c B_75_BPS
	- \c B_110_BPS
	- \c B_134_BPS
	- \c B_150_BPS
	- \c B_200_BPS
	- \c B_300_BPS
	- \c B_600_BPS
	- \c B_1200_BPS
	- \c B_1800_BPS
	- \c B_2400_BPS
	- \c B_4800_BPS
	- \c B_9600_BPS
	- \c B_19200_BPS
	- \c B_38400_BPS
	- \c B_57600_BPS
	- \c B_115200_BPS
	- \c B_230400_BPS
	- \c B_31250_BPS
	\return
	- \c B_OK if all goes fine,
	- an error code if something goes wrong.
*/
status_t
BSerialPort::SetDataRate(data_rate bitsPerSecond)
{
	fBaudRate = bitsPerSecond;

	return _DriverControl();
}


/*! \brief Get the current Baud Rate.
	\return The current Baud Rate.
*/

data_rate
BSerialPort::DataRate(void)
{
	return fBaudRate;
}


/*! \brief Set the data bits (7 or 8)
*/
void
BSerialPort::SetDataBits(data_bits numBits)
{
	fDataBits = numBits;
	_DriverControl();
}


/*! \brief Get the current data bits.
 	\return The current data bits.
*/
data_bits
BSerialPort::DataBits(void)
{
	return fDataBits;
}


/*! \brief Set the stop bits.
	\param numBits The number of stop bits
	Valid values:
	- \c B_STOP_BITS_1 (or \c B_STOP_BIT_1)
	- \c B_STOP_BITS_2
*/
void
BSerialPort::SetStopBits(stop_bits numBits)
{
	fStopBits = numBits;
	_DriverControl();
}


/*! \brief Get the current stop bits.
	\return The current stop bits.
*/
stop_bits
BSerialPort::StopBits(void)
{
	return fStopBits;
}


/*! \brief Set the parity mode.
	\param which The parity mode to set.
	Valid values:
	- \c B_ODD_PARITY
	- \c B_EVEN_PARITY
	- \c B_NO_PARITY
*/
void
BSerialPort::SetParityMode(parity_mode which)
{
	fParityMode = which;
	_DriverControl();
}


/*! \brief Get the parity mode.
	\return The current parity mode.
*/
parity_mode
BSerialPort::ParityMode(void)
{
	return fParityMode;
}


/*! \brief Clear the input buffer.
*/
void
BSerialPort::ClearInput(void)
{
	tcflush(ffd, TCIFLUSH);
}


/*! \brief Clear the output buffer.
*/
void
BSerialPort::ClearOutput(void)
{
	tcflush(ffd, TCOFLUSH);
}


/*! \brief Set the flow control
	\param method The type of flow control.
	Valid values:
	- \c B_HARDWARE_CONTROL
	- \c B_SOFTWARE_CONTROL
	- \c B_NOFLOW_CONTROL
*/
void
BSerialPort::SetFlowControl(uint32 method)
{
	fFlow = method;
	_DriverControl();
}


/*! \brief Returns the selected flow control.
	\return The flow control for the current open port.
*/
uint32
BSerialPort::FlowControl(void)
{
	return fFlow;
}


/* Set the DTR */
status_t
BSerialPort::SetDTR(bool asserted)
{
	status_t status = ioctl(ffd, TCSETDTR, &asserted, sizeof(asserted));

	return (status >= 0) ? status : errno;
}


/* Set the RTS status */
status_t
BSerialPort::SetRTS(bool asserted)
{
	status_t status = ioctl(ffd, TCSETRTS, &asserted, sizeof(asserted));

	return (status >= 0) ? status : errno;
}


/*! \brief See how many chars are queued on the serial port.
	\param numChars A pointer to an int32 where you want
		that value stored.
	\return ?
*/
status_t
BSerialPort::NumCharsAvailable(int32 *numChars)
{
	//No help from the BeBook...
	if (ffd < 0)
		return B_NO_INIT;

	// TODO: Implement ?
	if (numChars)
		*numChars = 0;
	return B_OK;
}


/*! \brief See if the Clear to Send pin is asserted.
	\return true if CTS is asserted, false if not.
*/
bool
BSerialPort::IsCTS(void)
{
	unsigned int bits = ioctl(ffd, TCGETBITS, 0);

	if (bits & TCGB_CTS)
		return true;

	return false;
}


/*! \brief See if the Data Set Ready pin is asserted.
	\return true if DSR is asserted, false if not.
*/
bool
BSerialPort::IsDSR(void)
{
	unsigned int bits = ioctl(ffd, TCGETBITS, 0);

	if (bits & TCGB_DSR)
		return true;

	return false;
}


/*! \brief See if the Ring Indicator pin is asserted.
	\return true if RI is asserted, false if not.
*/
bool
BSerialPort::IsRI(void)
{
	unsigned int bits = ioctl(ffd, TCGETBITS, 0);

	if (bits & TCGB_RI)
		return true;

	return false;
}


/*! \brief See if the Data Carrier Detect pin is asserted.
	\return true if DCD is asserted, false if not.
*/
bool
BSerialPort::IsDCD(void)
{
	unsigned int bits = ioctl(ffd, TCGETBITS, 0);

	if (bits & TCGB_DCD)
		return true;

	return false;
}


/*! \brief Wait until there's something to read from the serial port.
	If no data is ready, it will always block, ignoring the
	value of SetBlocking(); however, it respects the timeout
	set by SetTimeout().
	\return The number of bytes available to be read.
*/
ssize_t
BSerialPort::WaitForInput(void)
{
	ssize_t size;
	int err = ioctl(ffd, TCWAITEVENT, &size, sizeof size);

	return (err < B_OK) ? errno : size;
}


/*! \brief Count the number of available Serial Ports.
	\return An integer which represents the number of available
		serial ports.
*/
int32
BSerialPort::CountDevices()
{
	int32 count = 0;

	// Refresh devices list
	_ScanDevices();

	if (fDevices != NULL)
		count = fDevices->CountItems();

	return count;
}


/*! \brief Get the device name for the given device.
	\param n Number of the device you want to know the name of.
	\param name The buffer where you want to store the name.
	\param bufSize The size of the buffer.
	\return
	- \c B_ERROR if something goes wrong
	- \c B_OK if all goes fine.
*/
status_t
BSerialPort::GetDeviceName(int32 n, char *name, size_t bufSize)
{
	status_t result = B_ERROR;
	const char *dev = NULL;

	if (fDevices != NULL)
		dev = static_cast<char*>(fDevices->ItemAt(n));

	if (dev != NULL && name != NULL) {
		strncpy(name, dev, bufSize);
		name[bufSize - 1] = '\0';
		result = B_OK;
	}
	return result;
}


/* Private or Reserved */

/*! \brief Build a list of available serial ports.
	Query the serial driver about the available devices,
	and build a list of them.
*/
void
BSerialPort::_ScanDevices()
{
	// First, we empty the list
	if (fDevices != NULL) {
		for (int32 count = fDevices->CountItems() - 1; count >= 0; count--)
			free(fDevices->RemoveItem(count));

		// Add devices to the list
		scan_directory(SERIAL_DIR, fDevices);
	}
}


/*! \brief Send the selected options to the serial driver.
	\return
	- \c B_OK if all goes fine,
	- an error code if something goes wrong.
*/
int
BSerialPort::_DriverControl()
{
	struct termios options;
	int err;

	if (ffd < 0)
		return B_NO_INIT;

	//Load the current settings
	err = tcgetattr(ffd, &options);
	if (err < 0)
		return errno;

	// Reset all flags
	options.c_iflag &= ~(IXON | IXOFF | IXANY | INPCK);
	options.c_cflag &= ~(CRTSCTS | CSIZE | CSTOPB | PARODD | PARENB);
	options.c_lflag &= ~(ECHO | ECHONL | ISIG | ICANON);

	// Local line
	options.c_cflag |= CLOCAL;

	//Set the flags to the wanted values
	if (fFlow & B_HARDWARE_CONTROL)
		options.c_cflag |= CRTSCTS;

	if (fFlow & B_SOFTWARE_CONTROL)
		options.c_iflag |= (IXON | IXOFF);

	if (fStopBits & B_STOP_BITS_2)
		options.c_cflag |= CSTOPB; // Set 2 stop bits

	if (fDataBits & B_DATA_BITS_8)
		options.c_cflag |= CS8; // Set 8 data bits

	//Ok, set the parity now
	if (fParityMode != B_NO_PARITY) {
		options.c_cflag |= PARENB; //Enable parity
		if (fParityMode == B_ODD_PARITY)
			options.c_cflag |= PARODD; //Select odd parity
	}

	//Set the baud rate
	cfsetispeed(&options, fBaudRate);
	cfsetospeed(&options, fBaudRate);

	//Set the timeout
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 0;
	if (fBlocking) {
		if (fTimeout == B_INFINITE_TIMEOUT) {
			options.c_cc[VMIN] = 1;
		} else if (fTimeout != 0) {
			int timeout = fTimeout / 100000;
			options.c_cc[VTIME] = (timeout == 0) ? 1 : timeout;
		}
	}

	//Ok, finished. Now tell the driver what we decided
	err = tcsetattr(ffd, TCSANOW, &options);

	return (err >= 0) ? err : errno;
}


/* These functions are here to maintain Binary Compatibility */
void BSerialPort::_ReservedSerialPort1() {}
void BSerialPort::_ReservedSerialPort2() {}
void BSerialPort::_ReservedSerialPort3() {}
void BSerialPort::_ReservedSerialPort4() {}
