/**
 * @file MidiPort.cpp
 *
 * @author Matthijs Hollemans
 */

#include "debug.h"
#include "MidiPort.h"

//------------------------------------------------------------------------------

BMidiPort::BMidiPort(const char* name)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

BMidiPort::~BMidiPort()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

status_t BMidiPort::InitCheck() const
{
	UNIMPLEMENTED
	return B_NO_INIT;
}

//------------------------------------------------------------------------------

status_t BMidiPort::Open(const char* name)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

void BMidiPort::Close()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

const char* BMidiPort::PortName() const
{
	UNIMPLEMENTED
	return NULL;
}

//------------------------------------------------------------------------------

void BMidiPort::NoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiPort::NoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiPort::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiPort::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiPort::ProgramChange(
	uchar channel, uchar programNumber, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiPort::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiPort::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiPort::SystemExclusive(void* data, size_t dataLength, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiPort::SystemCommon(
	uchar status, uchar data1, uchar data2, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiPort::SystemRealTime(uchar status, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

status_t BMidiPort::Start()
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

void BMidiPort::Stop()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

int32 BMidiPort::CountDevices()
{
	UNIMPLEMENTED
	return 0;
}

//------------------------------------------------------------------------------

status_t BMidiPort::GetDeviceName(int32 n, char* name, size_t bufSize)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

void BMidiPort::_ReservedMidiPort1() { }
void BMidiPort::_ReservedMidiPort2() { }
void BMidiPort::_ReservedMidiPort3() { }

//------------------------------------------------------------------------------

void BMidiPort::Run()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiPort::Dispatch(
	const unsigned char* buffer, size_t size, bigtime_t when)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

ssize_t BMidiPort::Read(void* buffer, size_t numBytes) const
{
	UNIMPLEMENTED
	return 0;
}

//------------------------------------------------------------------------------

ssize_t BMidiPort::Write(void* buffer, size_t numBytes, uint32 time) const
{
	UNIMPLEMENTED
	return 0;
}

//------------------------------------------------------------------------------

void BMidiPort::ScanDevices()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

