/*
 * Copyright (c) 2002-2003 Matthijs Hollemans
 * Copyright (c) 2002 Jerome Leveque
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include <MidiProducer.h>
#include <MidiRoster.h>
#include <stdlib.h>

#include "debug.h"
#include "MidiPort.h"
#include "MidiGlue.h"

using namespace BPrivate;

//------------------------------------------------------------------------------

BMidiPort::BMidiPort(const char* name)
{
	portName = NULL;
	devices  = new BList;
	status   = B_ERROR;

	localSource = new BMidiLocalProducer("MidiPortGlue(out)");
	localSink   = new BMidiPortGlue(this, "MidiPortGlue(in)");

	remoteSource = NULL;
	remoteSink   = NULL;

	ScanDevices();

	if (name != NULL)
	{
		Open(name);
	}
}

//------------------------------------------------------------------------------

BMidiPort::~BMidiPort()
{
	Close();

	EmptyDeviceList();
	delete devices;

	localSource->Release();
	localSink->Release();
}

//------------------------------------------------------------------------------

status_t BMidiPort::InitCheck() const
{
	return status;
}

//------------------------------------------------------------------------------

status_t BMidiPort::Open(const char* name)
{
	status = B_ERROR;

	if (name != NULL)
	{
		Close();

		for (int32 t = 0; t < devices->CountItems(); ++t)
		{
			BMidiEndpoint* endp = (BMidiEndpoint*) devices->ItemAt(t);
			if (strcmp(name, endp->Name()) == 0)
			{
				if (endp->IsValid())  // still exists?
				{
					if (endp->IsProducer())
					{
						if (remoteSource == NULL)
						{
							remoteSource = (BMidiProducer*) endp;
						}
					}
					else if (endp->IsConsumer())
					{
						if (remoteSink == NULL)
						{
							remoteSink = (BMidiConsumer*) endp;
							localSource->Connect(remoteSink);
						}
					}
				}
			}
		}

		if (remoteSource != NULL)
		{
			portName = strdup(remoteSource->Name());
			status = B_OK;
		}
		else if (remoteSink != NULL)
		{
			portName = strdup(remoteSink->Name());
			status = B_OK;
		}
	}

	return status;
}

//------------------------------------------------------------------------------

void BMidiPort::Close()
{
	if (remoteSource != NULL)
	{
		remoteSource->Disconnect(localSink);
		remoteSource = NULL;
	}

	if (remoteSink != NULL)
	{
		localSource->Disconnect(remoteSink);
		remoteSink = NULL;
	}

	if (portName != NULL)
	{
		free(portName);
		portName = NULL;
	}
}

//------------------------------------------------------------------------------

const char* BMidiPort::PortName() const
{
	return portName;
}

//------------------------------------------------------------------------------

void BMidiPort::NoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	localSource->SprayNoteOff(channel - 1, note, velocity, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidiPort::NoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	localSource->SprayNoteOn(channel - 1, note, velocity, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidiPort::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	localSource->SprayKeyPressure(
		channel - 1, note, pressure, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidiPort::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	localSource->SprayControlChange(
		channel - 1, controlNumber, controlValue, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidiPort::ProgramChange(
	uchar channel, uchar programNumber, uint32 time)
{
	localSource->SprayProgramChange(
		channel - 1, programNumber, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidiPort::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	localSource->SprayChannelPressure(
		channel - 1, pressure, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidiPort::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	localSource->SprayPitchBend(channel - 1, lsb, msb, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidiPort::SystemExclusive(void* data, size_t length, uint32 time)
{
	localSource->SpraySystemExclusive(data, length, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidiPort::SystemCommon(
	uchar status, uchar data1, uchar data2, uint32 time)
{
	localSource->SpraySystemCommon(status, data1, data2, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

void BMidiPort::SystemRealTime(uchar status, uint32 time)
{
	localSource->SpraySystemRealTime(status, MAKE_BIGTIME(time));
}

//------------------------------------------------------------------------------

status_t BMidiPort::Start()
{
	status_t err = super::Start();

	if ((err == B_OK) && (remoteSource != NULL))
	{
		return remoteSource->Connect(localSink);
	}

	return err;
}

//------------------------------------------------------------------------------

void BMidiPort::Stop()
{
	if (remoteSource != NULL)
	{
		remoteSource->Disconnect(localSink);
	}

	super::Stop();
}

//------------------------------------------------------------------------------

int32 BMidiPort::CountDevices()
{
	return devices->CountItems();
}

//------------------------------------------------------------------------------

status_t BMidiPort::GetDeviceName(int32 n, char* name, size_t bufSize)
{
	BMidiEndpoint* endp = (BMidiEndpoint*) devices->ItemAt(n);
	if (endp == NULL)
	{
		return B_BAD_VALUE;
	}

	size_t size = strlen(endp->Name());
	if (size >= bufSize)
	{
		return B_NAME_TOO_LONG;
	}

	strcpy(name, endp->Name());
	return B_OK;
}

//------------------------------------------------------------------------------

void BMidiPort::_ReservedMidiPort1() { }
void BMidiPort::_ReservedMidiPort2() { }
void BMidiPort::_ReservedMidiPort3() { }

//------------------------------------------------------------------------------

void BMidiPort::Run()
{
	while (KeepRunning())
	{
		snooze(50000);
	}
}

//------------------------------------------------------------------------------

void BMidiPort::ScanDevices()
{
	EmptyDeviceList();

	int32 id = 0;
	BMidiEndpoint* endp;

	while ((endp = BMidiRoster::NextEndpoint(&id)) != NULL)
	{
		// Each hardware port has two endpoints associated with it, a consumer
		// and a producer. Both have the same name, so we add only one of them.

		bool addItem = true;
		for (int32 t = 0; t < devices->CountItems(); ++t)
		{
			BMidiEndpoint* other = (BMidiEndpoint*) devices->ItemAt(t);
			if (strcmp(endp->Name(), other->Name()) == 0)
			{
				addItem = false;
				break;
			}
		}

		if (addItem)
		{
			devices->AddItem(endp);
		}
		else
		{
			endp->Release();
		}
	}
}

//------------------------------------------------------------------------------

void BMidiPort::EmptyDeviceList()
{
	for (int32 t = 0; t < devices->CountItems(); ++t)
	{
		((BMidiEndpoint*) devices->ItemAt(t))->Release();
	}

	devices->MakeEmpty();
}

//------------------------------------------------------------------------------
