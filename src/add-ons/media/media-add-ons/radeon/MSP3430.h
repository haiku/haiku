/******************************************************************************
/
/	File:			MSP3430.h
/
/	Description:	Micronas Multistandard Sound Processor (MSP) interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#ifndef __MSP3430_H__
#define __MSP3430_H__

#include "I2CPort.h"

class CMSP3430 {
public:
	CMSP3430(CI2CPort & port);
	
	~CMSP3430();
	
	status_t InitCheck() const;

	void SetEnable(bool enable);
	
private:
	int ControlRegister();
	
	void SetControlRegister(int value);

	int Register(int address, int subaddress);
	
	void SetRegister(int address, int subaddress, int value);
		
private:
	CI2CPort & fPort;
	int fAddress;
};

#endif
