/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <SerialPort.h>

BSerialPort::BSerialPort()
{
}


BSerialPort::~BSerialPort()
{
}


status_t
BSerialPort::Open(const char *portName)
{
	return B_ERROR;
}


void
BSerialPort::Close(void)
{
}


ssize_t
BSerialPort::Read(void *buf,
				  size_t count)
{
	return 0;
}


ssize_t
BSerialPort::Write(const void *buf,
				   size_t count)
{
	return 0;
}


void
BSerialPort::SetBlocking(bool Blocking)
{
}


status_t
BSerialPort::SetTimeout(bigtime_t microSeconds)
{
	return B_ERROR;
}


status_t
BSerialPort::SetDataRate(data_rate bitsPerSecond)
{
	return B_ERROR;
}


data_rate
BSerialPort::DataRate(void)
{
	return B_0_BPS;
}


void
BSerialPort::SetDataBits(data_bits numBits)
{
}


data_bits
BSerialPort::DataBits(void)
{
	return B_DATA_BITS_8;
}


void
BSerialPort::SetStopBits(stop_bits numBits)
{
}


stop_bits
BSerialPort::StopBits(void)
{
	return B_STOP_BITS_1;
}


void
BSerialPort::SetParityMode(parity_mode which)
{
}


parity_mode
BSerialPort::ParityMode(void)
{
	return B_NO_PARITY;
}


void
BSerialPort::ClearInput(void)
{
}


void
BSerialPort::ClearOutput(void)
{
}


void
BSerialPort::SetFlowControl(uint32 method)
{
}


uint32
BSerialPort::FlowControl(void)
{
	return 0;
}


status_t
BSerialPort::SetDTR(bool asserted)
{
	return B_ERROR;
}


status_t
BSerialPort::SetRTS(bool asserted)
{
	return B_ERROR;
}


status_t
BSerialPort::NumCharsAvailable(int32 *wait_until_this_many)
{
	return B_ERROR;
}


bool
BSerialPort::IsCTS(void)
{
	return false;
}


bool
BSerialPort::IsDSR(void)
{
	return false;
}


bool
BSerialPort::IsRI(void)
{
	return false;
}


bool
BSerialPort::IsDCD(void)
{
	return false;
}


ssize_t
BSerialPort::WaitForInput(void)
{
	return 0;
}


int32
BSerialPort::CountDevices()
{
	return 0;
}


status_t
BSerialPort::GetDeviceName(int32 n,
						   char *name,
						   size_t bufSize)
{
	return B_ERROR;
}


void
BSerialPort::ScanDevices()
{
}


void
BSerialPort::_ReservedSerialPort1()
{
}


void
BSerialPort::_ReservedSerialPort2()
{
}


void
BSerialPort::_ReservedSerialPort3()
{
}


void
BSerialPort::_ReservedSerialPort4()
{
}


int
BSerialPort::DriverControl()
{
	return 0;
}


