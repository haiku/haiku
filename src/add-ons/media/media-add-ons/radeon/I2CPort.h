/******************************************************************************
/
/	File:			I2C.h
/
/	Description:	ATI Radeon I2C Serial Bus interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#ifndef __I2C_PORT_H__
#define __I2C_PORT_H__

#include "Radeon.h"

enum i2c_port_clock_rate {
	C_RADEON_I2C_MIN_CLOCK_RATE		= 7500,
	C_RADEON_I2C_MAX_CLOCK_RATE		= 100000,
	C_RADEON_I2C_DEFAULT_CLOCK_RATE = 80000
};

class CI2CPort {
public:
	CI2CPort(CRadeon & radeon, int rate = C_RADEON_I2C_DEFAULT_CLOCK_RATE);
	
	~CI2CPort();
	
	status_t InitCheck() const;
	
	CRadeon & Radeon() const;
	
	bool Probe(int address);
	
public:	
	bool Write(int address, const char * buffer, int length);
	
	bool Read(int address, char * buffer, int length);

	bool Write(int address, const char * buffer, int length, char * output, int outlen);

public:	
	int Register(int address, int index);

	void SetRegister(int address, int index, int value);

private:
	int Send(int address, const char * buffer, int length, bool start, bool stop);

	int Receive(int address, char * buffer, int length, bool start, bool stop);
	
	int WaitAck();

	void Stop();
	
private:
	CRadeon & fRadeon;
	int fNfactor;
	int fMfactor;
	int fTimeLimit;
	shared_info* si;
};


#endif
