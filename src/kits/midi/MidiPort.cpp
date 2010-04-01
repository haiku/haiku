/*
 * Copyright 2006, Haiku.
 * 
 * Copyright (c) 2002-2003 Matthijs Hollemans
 * Copyright (c) 2002 Jerome Leveque
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 *		Jérôme Leveque
 */

#include <MidiPort.h>
#include <MidiProducer.h>
#include <MidiRoster.h>
#include <stdlib.h>

#include "debug.h"
#include "MidiGlue.h"

using namespace BPrivate;


BMidiPort::BMidiPort(const char* name)
{
	fPortName = NULL;
	fDevices  = new BList;
	fStatus   = B_ERROR;

	fLocalSource = new BMidiLocalProducer("MidiPortGlue(out)");
	fLocalSink   = new BMidiPortGlue(this, "MidiPortGlue(in)");
	fLocalSource->Register();
	fLocalSink->Register();

	fRemoteSource = NULL;
	fRemoteSink   = NULL;

	ScanDevices();

	if (name != NULL)
		Open(name);
}


BMidiPort::~BMidiPort()
{
	Close();

	EmptyDeviceList();
	delete fDevices;

	fLocalSource->Unregister();
	fLocalSink->Unregister();
	fLocalSource->Release();
	fLocalSink->Release();
}


status_t 
BMidiPort::InitCheck() const
{
	return fStatus;
}


status_t 
BMidiPort::Open(const char* name)
{
	fStatus = B_ERROR;

	if (name != NULL) {
		Close();

		for (int32 t = 0; t < fDevices->CountItems(); ++t) {
			BMidiEndpoint* endp = (BMidiEndpoint*) fDevices->ItemAt(t);
			if (strcmp(name, endp->Name()) != 0)
				continue;
			if (!endp->IsValid())  // still exists?
				continue;
			if (endp->IsProducer()) {
				if (fRemoteSource == NULL)
					fRemoteSource = (BMidiProducer*) endp;
			} else {
				if (fRemoteSink == NULL) {
					fRemoteSink = (BMidiConsumer*) endp;
					fLocalSource->Connect(fRemoteSink);
				}
			}
		}

		if (fRemoteSource != NULL) {
			fPortName = strdup(fRemoteSource->Name());
			fStatus = B_OK;
		} else if (fRemoteSink != NULL) {
			fPortName = strdup(fRemoteSink->Name());
			fStatus = B_OK;
		}
	}

	return fStatus;
}


void 
BMidiPort::Close()
{
	if (fRemoteSource != NULL) {
		fRemoteSource->Disconnect(fLocalSink);
		fRemoteSource = NULL;
	}

	if (fRemoteSink != NULL) {
		fLocalSource->Disconnect(fRemoteSink);
		fRemoteSink = NULL;
	}

	if (fPortName != NULL) {
		free(fPortName);
		fPortName = NULL;
	}
}


const char* 
BMidiPort::PortName() const
{
	return fPortName;
}


void 
BMidiPort::NoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	fLocalSource->SprayNoteOff(channel - 1, note, velocity, MAKE_BIGTIME(time));
}


void 
BMidiPort::NoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	fLocalSource->SprayNoteOn(channel - 1, note, velocity, MAKE_BIGTIME(time));
}


void 
BMidiPort::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	fLocalSource->SprayKeyPressure(
		channel - 1, note, pressure, MAKE_BIGTIME(time));
}


void 
BMidiPort::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	fLocalSource->SprayControlChange(
		channel - 1, controlNumber, controlValue, MAKE_BIGTIME(time));
}


void 
BMidiPort::ProgramChange(
	uchar channel, uchar programNumber, uint32 time)
{
	fLocalSource->SprayProgramChange(
		channel - 1, programNumber, MAKE_BIGTIME(time));
}


void 
BMidiPort::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	fLocalSource->SprayChannelPressure(
		channel - 1, pressure, MAKE_BIGTIME(time));
}


void 
BMidiPort::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	fLocalSource->SprayPitchBend(channel - 1, lsb, msb, MAKE_BIGTIME(time));
}


void 
BMidiPort::SystemExclusive(void* data, size_t length, uint32 time)
{
	fLocalSource->SpraySystemExclusive(data, length, MAKE_BIGTIME(time));
}


void 
BMidiPort::SystemCommon(
	uchar status, uchar data1, uchar data2, uint32 time)
{
	fLocalSource->SpraySystemCommon(status, data1, data2, MAKE_BIGTIME(time));
}


void 
BMidiPort::SystemRealTime(uchar status, uint32 time)
{
	fLocalSource->SpraySystemRealTime(status, MAKE_BIGTIME(time));
}


status_t 
BMidiPort::Start()
{
	status_t err = super::Start();

	if ((err == B_OK) && (fRemoteSource != NULL)) {
		return fRemoteSource->Connect(fLocalSink);
	}

	return err;
}


void 
BMidiPort::Stop()
{
	if (fRemoteSource != NULL) {
		fRemoteSource->Disconnect(fLocalSink);
	}

	super::Stop();
}


int32 
BMidiPort::CountDevices()
{
	return fDevices->CountItems();
}


status_t 
BMidiPort::GetDeviceName(int32 n, char* name, size_t bufSize)
{
	BMidiEndpoint* endp = (BMidiEndpoint*) fDevices->ItemAt(n);
	if (endp == NULL)
		return B_BAD_VALUE;

	size_t size = strlen(endp->Name());
	if (size >= bufSize)
		return B_NAME_TOO_LONG;

	strcpy(name, endp->Name());
	return B_OK;
}


void BMidiPort::_ReservedMidiPort1() { }
void BMidiPort::_ReservedMidiPort2() { }
void BMidiPort::_ReservedMidiPort3() { }


void 
BMidiPort::Run()
{
	while (KeepRunning()) 
		snooze(50000);
}


void 
BMidiPort::ScanDevices()
{
	EmptyDeviceList();

	int32 id = 0;
	BMidiEndpoint* endp;

	while ((endp = BMidiRoster::NextEndpoint(&id)) != NULL) {
		// Each hardware port has two endpoints associated with it, a consumer
		// and a producer. Both have the same name, so we add only one of them.

		bool addItem = true;
		for (int32 t = 0; t < fDevices->CountItems(); ++t) {
			BMidiEndpoint* other = (BMidiEndpoint*) fDevices->ItemAt(t);
			if (strcmp(endp->Name(), other->Name()) == 0) {
				addItem = false;
				break;
			}
		}

		if (addItem) {
			fDevices->AddItem(endp);
		} else {
			endp->Release();
		}
	}
}


void 
BMidiPort::EmptyDeviceList()
{
	for (int32 t = 0; t < fDevices->CountItems(); ++t) 
		((BMidiEndpoint*) fDevices->ItemAt(t))->Release();

	fDevices->MakeEmpty();
}

