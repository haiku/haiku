/**
 * @file MidiPort.cpp
 * 
 * @author Jerome Leveque
 */

//----------------------------------------

#include "MidiPortConsumer.cpp" 

//----------------------------------------

#include <MidiPort.h>
#include <MidiRoster.h>
#include <string.h>

#include <MidiProducer.h>

//#define DEBUG

//----------------------------------------
//Public function

BMidiPort::BMidiPort(const char *name = NULL)
	: BMidi(),
	remote_source(NULL),
	remote_sink(NULL),
	fCStatus(B_ERROR),
	_fDevices(new BList)
{
	local_source = new BMidiLocalProducer("MidiPortGlue(out)");
	if (local_source != NULL)
		local_source->Register();
	local_sink = new BMidiPortConsumer(this, "MidiPortGlue(in)");
	if (local_sink != NULL)
		local_sink->Register();

	ScanDevices();
	Open(name);
}

//-----------------

BMidiPort::~BMidiPort()
{
	Close();
	delete fName;
	if (local_source)
		local_source->Release();
	if (local_sink)
		local_sink->Release();
}

//----------------------------------------

status_t BMidiPort::InitCheck() const
{
return fCStatus;
}

//-----------------

status_t BMidiPort::Open(const char *name)
{
	if (name == NULL)
		return fCStatus = B_ERROR;

	if (remote_source != NULL)
	{
		if (local_sink != NULL)
		{
			remote_source->Disconnect(local_sink);
		}
		remote_source->Release();
		remote_source = NULL;
	}
	if (remote_sink != NULL)
	{
		if (local_source != NULL)
		{
			local_source->Disconnect(remote_sink);
		}
		remote_sink->Release();
		remote_sink = NULL;
	}
	
int32 id = 0;
	BMidiRoster *roster = BMidiRoster::MidiRoster();
	if (roster == NULL)
		return fCStatus = B_ERROR;
	BMidiEndpoint *endpt = NULL;
	while ((endpt = roster->NextEndpoint(&id)) != NULL)
	{
		if (strcmp(endpt->Name(), name) == 0)
		{//An endpoint have the same name as we want
			fName = new char[strlen(name)];
			if (fName != NULL)
				strcpy(fName, name);
			if (endpt->IsProducer())
			{
				remote_source = (BMidiProducer*)endpt;
				endpt->Acquire(); //Because we always call Release() 
			}
			if (endpt->IsConsumer())
			{
				remote_sink = (BMidiConsumer*)endpt;
				endpt->Acquire();//Because we always call Release() 
				if (local_source != NULL)
					local_source->Connect(remote_sink);
			}
		}
	endpt->Release();
	}
return fCStatus = B_OK;
}

//-----------------

void BMidiPort::Close()
{
	if ((local_sink != NULL) && (remote_source != NULL))
	{
		remote_source->Disconnect(local_sink);
		remote_source->Release();
		remote_source = NULL;
	}
	if ((local_source != NULL) && (remote_sink != NULL))
	{
		local_source->Disconnect(remote_sink);
		remote_sink->Release();
		remote_sink = NULL;
	}
}

//-----------------

int32 BMidiPort::CountDevices()
{
return _fDevices->CountItems();
}

//-----------------

status_t BMidiPort::GetDeviceName(int32 n, char *name, size_t bufSize)
{
BMidiEndpoint *endpt = (BMidiEndpoint*)_fDevices->ItemAt(n);
	if (endpt == NULL)
		return B_BAD_VALUE;
	size_t size = strlen(endpt->Name());
	if (size > bufSize)
		return B_NAME_TOO_LONG;
	if (strlen(strcpy(name, endpt->Name())) == size)
		return B_OK;
	else
		return B_ERROR;
}

//-----------------

const char *BMidiPort::PortName() const
{
return fName;
}

//----------------------------------------

void BMidiPort::NoteOff(uchar channel, uchar note, uchar velocity, uint32 time = B_NOW)
{
		local_source->SprayNoteOff(channel, note, velocity, 0);
}

//-----------------

void BMidiPort::NoteOn(uchar channel, uchar note, uchar velocity, uint32 time = B_NOW)
{
		local_source->SprayNoteOn(channel, note, velocity, 0);
}

//-----------------

void BMidiPort::KeyPressure(uchar channel, uchar note, uchar pressure, uint32 time = B_NOW)
{
		local_source->SprayKeyPressure(channel, note, pressure, 0);
}

//-----------------

void BMidiPort::ControlChange(uchar channel, uchar controlNumber, uchar controlValue, uint32 time = B_NOW)
{
		local_source->SprayControlChange(channel, controlNumber, controlValue, 0);
}

//-----------------

void BMidiPort::ProgramChange(uchar channel, uchar programNumber, uint32 time = B_NOW)
{
		local_source->SprayProgramChange(channel, programNumber, 0);
}

//-----------------

void BMidiPort::ChannelPressure(uchar channel, uchar pressure, uint32 time = B_NOW)
{
		local_source->SprayChannelPressure(channel, pressure, 0);
}

//-----------------

void BMidiPort::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time = B_NOW)
{
		local_source->SprayPitchBend(channel, lsb, msb, 0);
}

//-----------------

void BMidiPort::SystemExclusive(void* data, size_t dataLength, uint32 time = B_NOW)
{
		local_source->SpraySystemExclusive(data, dataLength, 0);
}

//-----------------

void BMidiPort::SystemCommon(uchar statusByte, uchar data1, uchar data2, uint32 time = B_NOW)
{
		local_source->SpraySystemCommon(statusByte, data1, data2, 0);
}

//-----------------

void BMidiPort::SystemRealTime(uchar statusByte, uint32 time = B_NOW)
{
		local_source->SpraySystemRealTime(statusByte, 0);
}

//----------------------------------------

status_t BMidiPort::Start()
{
	if ((local_sink != NULL) && (remote_source != NULL))
		if ((fCStatus =  remote_source->Connect(local_sink)) != B_OK)
			return fCStatus;
		else
		{
			return BMidi::Start();
		}
	else
		return fCStatus = B_ERROR;
}

//-----------------

void BMidiPort::Stop()
{
	if ((remote_source != NULL) && (local_sink != NULL))
	{
		remote_source->Disconnect(local_sink);
	}
	BMidi::Stop();
}

//----------------------------------------
//Private Function

void BMidiPort::_ReservedMidiPort1() {}
void BMidiPort::_ReservedMidiPort2() {}
void BMidiPort::_ReservedMidiPort3() {}

//----------------------------------------
//Not Used function

void BMidiPort::Dispatch(const unsigned char *buffer, size_t size, bigtime_t when) {}
ssize_t BMidiPort::Read(void *buffer, size_t numBytes) const { return 0; }
ssize_t BMidiPort::Write(void *buffer, size_t numBytes, uint32 time) const { return 0; }

//-----------------
//Used function

void BMidiPort::Run()
{
	while(KeepRunning())
		snooze(50000);
}

//-----------------

void BMidiPort::ScanDevices()
{
int32 id = 0;
	BMidiRoster *roster = BMidiRoster::MidiRoster();
	if (roster == NULL)
		return;
	BMidiEndpoint *endpt = NULL;
	while ((endpt = roster->NextEndpoint(&id)) != NULL)
	{
		bool additem = true; 
		for (int32 i = 0; i < _fDevices->CountItems(); i++)
		{
			BMidiEndpoint *endpoint = (BMidiEndpoint*)_fDevices->ItemAt(i);
			if (strcmp(endpt->Name(), endpoint->Name()) == 0)
				additem = false;
		}
		if (additem)
			_fDevices->AddItem(endpt);
		endpt->Release();
	}
}

//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
//----------------------------------------
