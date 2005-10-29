/********************************************************************************
/
/	File:		SerialPort.h
/
/	Description:	Serial Port class header.
/
/	Copyright 1995-98, Be Incorporated, All Rights Reserved.
/
********************************************************************************/


#ifndef	_SERIAL_PORT_H
#define	_SERIAL_PORT_H

#include <BeBuild.h>
#include <stddef.h>
#include <OS.h>
#include <SupportDefs.h>

class BList;

enum data_rate { B_0_BPS = 0, B_50_BPS, B_75_BPS, B_110_BPS, B_134_BPS,
				 B_150_BPS, B_200_BPS, B_300_BPS, B_600_BPS, B_1200_BPS,
				 B_1800_BPS, B_2400_BPS, B_4800_BPS, B_9600_BPS, B_19200_BPS,
				 B_38400_BPS, B_57600_BPS, B_115200_BPS, 
				 B_230400_BPS, B_31250_BPS };

enum data_bits { B_DATA_BITS_7, B_DATA_BITS_8 };

enum stop_bits { B_STOP_BITS_1, B_STOP_BITS_2 };
#define B_STOP_BIT_1	B_STOP_BITS_1

enum parity_mode { B_NO_PARITY, B_ODD_PARITY, B_EVEN_PARITY };

enum { B_NOFLOW_CONTROL = 0, B_HARDWARE_CONTROL = 0x00000001, 
								B_SOFTWARE_CONTROL = 0x00000002 };

/* -----------------------------------------------------------------------*/
class BSerialPort {

public:
					BSerialPort();
virtual				~BSerialPort();

		status_t	Open(const char *portName);
		void		Close(void);

		ssize_t		Read(void *buf, size_t count);
		ssize_t		Write(const void *buf, size_t count);
		void		SetBlocking(bool Blocking);
		status_t	SetTimeout(bigtime_t microSeconds);

		status_t	SetDataRate(data_rate bitsPerSecond);
		data_rate	DataRate(void);

		void		SetDataBits(data_bits numBits);
		data_bits	DataBits(void);
		void		SetStopBits(stop_bits numBits);
		stop_bits	StopBits(void);

		void		SetParityMode(parity_mode which);
		parity_mode	ParityMode(void);

		void		ClearInput(void);
		void		ClearOutput(void);

		void		SetFlowControl(uint32 method);
		uint32		FlowControl(void);

		status_t	SetDTR(bool asserted);
		status_t	SetRTS(bool asserted);
		status_t	NumCharsAvailable(int32 *wait_until_this_many);

		bool		IsCTS(void);
		bool		IsDSR(void);
		bool		IsRI(void);
		bool		IsDCD(void);
		ssize_t		WaitForInput(void);
/* -----------------------------------------------------------------------*/

		int32		CountDevices();
		status_t	GetDeviceName(int32 n, char * name, 
						size_t bufSize = B_OS_NAME_LENGTH);

private:

		void		ScanDevices();

virtual	void		_ReservedSerialPort1();
virtual	void		_ReservedSerialPort2();
virtual	void		_ReservedSerialPort3();
virtual	void		_ReservedSerialPort4();

		int			ffd;
		data_rate	fBaudRate;
		data_bits	fDataBits;
		stop_bits	fStopBits;
		parity_mode	fParityMode;
		uint32		fFlow;
		bigtime_t	fTimeout;
		bool		fBlocking;

		int			DriverControl();

		BList *		_fDevices;
		uint32		_fReserved[3];
};

#endif
