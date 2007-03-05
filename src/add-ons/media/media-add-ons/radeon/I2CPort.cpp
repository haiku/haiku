/******************************************************************************
/
/	File:			I2C.cpp
/
/	Description:	ATI Radeon I2C Serial Bus interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#include <Debug.h>
#include "I2CPort.h"

CI2CPort::CI2CPort(CRadeon & radeon, int rate)
	:	fRadeon(radeon),
		fNfactor(0),
		fMfactor(0),
		fTimeLimit(0),
		si(NULL)
{
	
	PRINT(("CI2CPort::CI2CPort()\n"));
	
	if( fRadeon.InitCheck() == B_OK ) {
		int refFreq, refDiv, minFreq, maxFreq, xclock;
		double n;
		
		fRadeon.GetPLLParameters(refFreq, refDiv, minFreq, maxFreq, xclock);
		si = fRadeon.GetSharedInfo();
		
		if ( si->asic == rt_rv200 ) {
			n = (xclock * 40000.0) / (1.0 * rate);
		} else {
			n = (xclock * 10000.0) / (4.0 * rate);
		}
		
		for (fNfactor = 1; fNfactor < 255; fNfactor++) {
			if (fNfactor * (fNfactor - 1) > n)
				break;
		}
		fMfactor = fNfactor - 1;
		fTimeLimit = 2 * fNfactor;
	
		// enable I2C bus
		fRadeon.SetRegister(C_RADEON_I2C_CNTL_1, 0x00ff0000,
			C_RADEON_I2C_SEL | C_RADEON_I2C_EN);
		
		fRadeon.SetRegister(C_RADEON_I2C_CNTL_0, 0x000000ff,
			C_RADEON_I2C_DONE | C_RADEON_I2C_NACK |
			C_RADEON_I2C_HALT | C_RADEON_I2C_SOFT_RST |
			C_RADEON_I2C_DRIVE_EN | C_RADEON_I2C_DRIVE_SEL);
			
#if DEBUG
		PRINT(("CI2CPort::CI2CPort() - I2C devices found at ports: "));
		for (int address = 0x80; address <= 0xff; address += 0x02) {
			if (Probe(address))
				PRINT(("0x%02x ", address));
		}
		PRINT(("\n"));
#endif
	}
}

CI2CPort::~CI2CPort()
{
	PRINT(("CI2CPort::~CI2CPort()\n"));
	if( fRadeon.InitCheck() == B_OK ) {
		// disable I2C bus
		fRadeon.SetRegister(C_RADEON_I2C_CNTL_1, 0);
	}
}

status_t CI2CPort::InitCheck() const
{
	if (fRadeon.InitCheck() != B_OK)
		return B_ERROR;
		
	if ( si == NULL )
		return B_ERROR;
	
	if ( si->has_no_i2c ) {
		PRINT(("This Chips I2C is BLACKLISTED!"));
		return B_ERROR;
	}	
	return B_OK;
}

CRadeon & CI2CPort::Radeon() const
{
	return fRadeon;
}

bool CI2CPort::Probe(int address)
{
	char buffer[1];
	
	return Read(address, buffer, sizeof(buffer));
}

bool CI2CPort::Write(int address, const char * buffer, int length)
{
	if (Send(address, buffer, length, true, true) == length)
		return true;
	return false;
}

bool CI2CPort::Read(int address, char * buffer, int length)
{
	if (Receive(address, buffer, length, true, true) == length)
		return true;
	return false;
}

bool CI2CPort::Write(int address, const char * buffer, int length, char * result, int reslen)
{
	if (Send(address, buffer, length, true, false) == length)
		if (Receive(address, result, reslen, true, true) == reslen)
			return true;
	return false;
}

int CI2CPort::Register(int address, int index)
{
	char value = index;

	if (Send(address, &value, sizeof(value), true, false) == sizeof(value)) {
		if (Receive(address, &value, sizeof(value), true, true) == sizeof(value))
			return value & 0xff;
	}
	PRINT(("CI2CPort::Register() - error\n"));
	return -1;
}

void CI2CPort::SetRegister(int address, int index, int value)
{
	char buffer[2];
	
	buffer[0] = index;
	buffer[1] = value;
	
	if (Send(address, buffer, sizeof(buffer), true, true) != sizeof(buffer))
		PRINT(("CI2CPort::SetRegister() - error\n"));
}

int CI2CPort::Send(int address, const char * buffer, int length, bool start, bool stop)
{
	//fRadeon.WaitForFifo(4 + length);
	
	// clear status bit
	fRadeon.SetRegister(C_RADEON_I2C_CNTL_0,
		C_RADEON_I2C_DONE |	C_RADEON_I2C_NACK |
		C_RADEON_I2C_HALT |	C_RADEON_I2C_SOFT_RST);

	// write address
	fRadeon.SetRegister(C_RADEON_I2C_DATA, address & ~(1));

	// write data
	for (int offset = 0; offset < length; offset++)
		fRadeon.SetRegister(C_RADEON_I2C_DATA, buffer[offset]);

	//
	if (si->asic >= rt_r200) {
		fRadeon.SetRegister(C_RADEON_I2C_CNTL_1,
			(fTimeLimit << 24) | length |
			C_RADEON_I2C_EN | C_RADEON_I2C_SEL | 0x010);
	} else {
		fRadeon.SetRegister(C_RADEON_I2C_CNTL_1,
			(fTimeLimit << 24) | length |
			C_RADEON_I2C_EN | C_RADEON_I2C_SEL | 0x100);
	}
	
	fRadeon.SetRegister(C_RADEON_I2C_CNTL_0,
		(fNfactor << 24) | (fMfactor << 16) |
		C_RADEON_I2C_GO | C_RADEON_I2C_DRIVE_EN |
		(start ? C_RADEON_I2C_START : 0) | (stop ? C_RADEON_I2C_STOP : 0));
		
	for (int wait = 0; wait < 100; wait++) {
		if ((fRadeon.Register(C_RADEON_I2C_CNTL_0) & C_RADEON_I2C_GO) == 0)
			break;
	}

	if (WaitAck() != C_RADEON_I2C_DONE) {
		Stop();
		return 0;
	}
	
	return length;
}

int CI2CPort::Receive(int address, char * buffer, int length, bool start, bool stop)
{
	//fRadeon.WaitForFifo(length + 4);
	
	fRadeon.SetRegister(C_RADEON_I2C_CNTL_0,
		C_RADEON_I2C_DONE | C_RADEON_I2C_NACK |
		C_RADEON_I2C_HALT | C_RADEON_I2C_SOFT_RST);
	
	fRadeon.SetRegister(C_RADEON_I2C_DATA, address | 0x00000001);
	
	if (si->asic >= rt_r200) {
		fRadeon.SetRegister(C_RADEON_I2C_CNTL_1,
			(fTimeLimit << 24) | C_RADEON_I2C_EN | C_RADEON_I2C_SEL |
			length | 0x010);
	} else {
		fRadeon.SetRegister(C_RADEON_I2C_CNTL_1,
			(fTimeLimit << 24) | C_RADEON_I2C_EN | C_RADEON_I2C_SEL |
			length | 0x100);
	}
		
	fRadeon.SetRegister(C_RADEON_I2C_CNTL_0,
		(fNfactor << 24) | (fMfactor << 16) | C_RADEON_I2C_GO |
		(start ? C_RADEON_I2C_START : 0) | (stop ? C_RADEON_I2C_STOP : 0) |
		C_RADEON_I2C_DRIVE_EN | C_RADEON_I2C_RECEIVE);
	
	
	for (int wait = 0; wait < 100; wait++) {
		if ((fRadeon.Register(C_RADEON_I2C_CNTL_0) & C_RADEON_I2C_GO) == 0)
			break;
	}

	if (WaitAck() != C_RADEON_I2C_DONE)
		return 0;

	snooze(1000);
	
	for (int offset = 0; offset < length; offset++) {
		//fRadeon.WaitForFifo(1);
		buffer[offset] = fRadeon.Register(C_RADEON_I2C_DATA) & 0xff;
	}

	return length;
}

int CI2CPort::WaitAck()
{
	for (int wait = 0; wait < 100; wait++) {
		int control = fRadeon.Register(C_RADEON_I2C_CNTL_0);
		if ((control & C_RADEON_I2C_HALT) != 0)
			return C_RADEON_I2C_HALT;
		if ((control & C_RADEON_I2C_NACK) != 0)
			return C_RADEON_I2C_NACK;
		if ((control & C_RADEON_I2C_DONE) != 0)
			return C_RADEON_I2C_DONE;
		snooze(1000);
	}
	PRINT(("CI2CPort::WaitAck() - Time out!\n"));
	return C_RADEON_I2C_HALT;		
}

void CI2CPort::Stop()
{
	// reset status flags
	fRadeon.SetRegister(C_RADEON_I2C_CNTL_0,
		C_RADEON_I2C_DONE | C_RADEON_I2C_NACK | C_RADEON_I2C_HALT, 0);

	// issue abort call
	fRadeon.SetRegister(C_RADEON_I2C_CNTL_0_PLUS1,
		C_RADEON_I2C_ABORT | C_RADEON_I2C_GO, C_RADEON_I2C_ABORT | C_RADEON_I2C_GO);
	
	// wait GO bit to go low
	for (int wait = 0; wait < 100; wait++) {
		if ((fRadeon.Register(C_RADEON_I2C_CNTL_0) & C_RADEON_I2C_GO) == 0)
		snooze(1000);
	}
	PRINT(("CI2CPort::Stop() - Time out!\n"));
}
