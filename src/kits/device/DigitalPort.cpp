/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <DigitalPort.h>

BDigitalPort::BDigitalPort()
{
}


BDigitalPort::~BDigitalPort()
{
}


status_t
BDigitalPort::Open(const char *portName)
{
	return B_ERROR;
}


void
BDigitalPort::Close(void)
{
}


bool
BDigitalPort::IsOpen(void)
{
	return false;
}


ssize_t
BDigitalPort::Read(uint8 *buf)
{
	return 0;
}


ssize_t
BDigitalPort::Write(uint8 value)
{
	return 0;
}


status_t
BDigitalPort::SetAsOutput(void)
{
	return B_ERROR;
}


bool
BDigitalPort::IsOutput(void)
{
	return false;
}


status_t
BDigitalPort::SetAsInput(void)
{
	return B_ERROR;
}


bool
BDigitalPort::IsInput(void)
{
	return false;
}


void
BDigitalPort::_ReservedDigitalPort1()
{
}


void
BDigitalPort::_ReservedDigitalPort2()
{
}


void
BDigitalPort::_ReservedDigitalPort3()
{
}


