/*
 * Copyright 2003-2006, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include <cstdio>

#include "ModemDevice.h"
#include "ACFCHandler.h"
#include "fcs.h"

#include <unistd.h>
#include <termios.h>
	// for port settings

// from libkernelppp
#include <settings_tools.h>


#if DEBUG
static char sDigits[] = "0123456789ABCDEF";
void
dump_packet(net_buffer *packet)
{
	if (!packet)
		return;

	uint8 *data = mtod(packet, uint8*);
	uint8 buffer[33];
	uint8 bufferIndex = 0;

	TRACE("Dumping packet;len=%ld;pkthdr.len=%d\n", packet->m_len,
		packet->m_flags & M_PKTHDR ? packet->m_pkthdr.len : -1);

	for (uint32 index = 0; index < packet->m_len; index++) {
		buffer[bufferIndex++] = sDigits[data[index] >> 4];
		buffer[bufferIndex++] = sDigits[data[index] & 0x0F];
		if (bufferIndex == 32 || index == packet->m_len - 1) {
			buffer[bufferIndex] = 0;
			TRACE("%s\n", buffer);
			bufferIndex = 0;
		}
	}
}
#endif


status_t
modem_put_line(int32 handle, const char *string, int32 length)
{
	char line[128];
	if (length > 126)
		return -1;

	sprintf(line, "%s\r", string);
	return write(handle, line, length + 1);
}


status_t
modem_get_line(int32 handle, char *string, int32 length, const char *echo)
{
	if (!string || length < 40)
		return -1;

	int32 result, position = 0;

	while(position < length) {
		result = read(handle, string + position, 1);
		if (result < 0)
			return -1;
		else if (result == 1) {
			if (string[position] == '\r') {
				string[position] = 0;
				if (!strcasecmp(string, echo)) {
					position = 0;
					continue;
				}

				return position;
			}

			position++;
		}
	}

	return -1;
}


static
status_t
worker_thread(void *data)
{
	ModemDevice *device = (ModemDevice*) data;
	int32 handle = device->Handle();
	uint8 buffer[MODEM_MTU];

	// send init string
	if (modem_put_line(handle, device->InitString(), strlen(device->InitString())) < 0
			|| modem_get_line(handle, (char*) buffer, sizeof(buffer),
					device->InitString()) < 0
			|| strcmp((char*) buffer, "OK")) {
		device->FailedDialing();
		return B_ERROR;
	}

	// send dial string
	if (modem_put_line(handle, device->DialString(), strlen(device->DialString())) < 0
			|| modem_get_line(handle, (char*) buffer, sizeof(buffer),
					device->DialString()) < 0
			|| strncmp((char*) buffer, "CONNECT", 7)) {
		device->FailedDialing();
		return B_ERROR;
	}

	if (strlen((char*) buffer) > 8)
		device->SetSpeed(atoi((char*) buffer + 8));
	else
		device->SetSpeed(19200);

	// TODO: authenticate if needed

	device->FinishedDialing();

	// start decoding
	int32 length = 0, position = 0;
	bool inPacket = true, needsEscape = false;

	while(true) {
		// ignore data if buffer is full
		if (position == MODEM_MTU)
			position = 0;

		length = read(handle, buffer + position, MODEM_MTU - position);

		if (length < 0 || !device->IsUp()) {
			device->ConnectionLost();
			return B_ERROR;
		}

		// decode the packet
		for (int32 index = 0; index < length; ) {
			if (buffer[position] == FLAG_SEQUENCE) {
				if (inPacket && position > 0)
					device->DataReceived(buffer, position);
						// DataReceived() will check FCS

				length = length - index - 1;
					// remaining data length
				memmove(buffer, buffer + position + 1, length);
				position = index = 0;

				needsEscape = false;
				inPacket = true;
				continue;
			}

			if (buffer[position + index] < 0x20) {
				++index;
				continue;
			}

			if (needsEscape) {
				buffer[position] = buffer[position + index] ^ 0x20;
				++position;
				needsEscape = false;
			} else if (buffer[position + index] == CONTROL_ESCAPE) {
				++index;
				needsEscape = true;
			} else {
				buffer[position] = buffer[position + index];
				++position;
			}
		}
	}
}


ModemDevice::ModemDevice(KPPPInterface& interface, driver_parameter *settings)
	: KPPPDevice("Modem", 0, interface, settings),
	fPortName(NULL),
	fHandle(-1),
	fWorkerThread(-1),
	fOutputBytes(0),
	fState(INITIAL)
{
#if DEBUG
	TRACE("ModemDevice: Constructor\n");
	if (!settings || !settings->parameters)
		TRACE("ModemDevice::ctor: No settings!\n");
#endif

	fACFC = new ACFCHandler(REQUEST_ACFC | ALLOW_ACFC, interface);
	if (!interface.LCP().AddOptionHandler(fACFC)) {
		fInitStatus = B_ERROR;
		return;
	}

	interface.SetPFCOptions(PPP_REQUEST_PFC | PPP_ALLOW_PFC);

	SetSpeed(19200);
	SetMTU(MODEM_MTU);
		// MTU size does not contain PPP header

	fPortName = get_parameter_value(MODEM_PORT_KEY, settings);
	fInitString = get_parameter_value(MODEM_INIT_KEY, settings);
	fDialString = get_parameter_value(MODEM_DIAL_KEY, settings);

	TRACE("ModemDevice::ctor: interfaceName: %s\n", fPortName);
}


ModemDevice::~ModemDevice()
{
	TRACE("ModemDevice: Destructor\n");
}


status_t
ModemDevice::InitCheck() const
{
	if (fState != INITIAL && Handle() == -1)
		return B_ERROR;

	return PortName() && InitString() && DialString()
		&& KPPPDevice::InitCheck() == B_OK ? B_OK : B_ERROR;
}


bool
ModemDevice::Up()
{
	TRACE("ModemDevice: Up()\n");

	if (InitCheck() != B_OK)
		return false;

	if (IsUp())
		return true;

	fState = INITIAL;
		// reset state

	// check if we are allowed to go up now (user intervention might disallow that)
	if (!UpStarted()) {
		CloseModem();
		DownEvent();
		return true;
			// there was no error
	}

	OpenModem();

	fState = DIALING;

	if (fWorkerThread == -1) {
		fWorkerThread = spawn_kernel_thread(worker_thread, "Modem: worker_thread",
			B_NORMAL_PRIORITY, this);
		resume_thread(fWorkerThread);
	}

	return true;
}


bool
ModemDevice::Down()
{
	TRACE("ModemDevice: Down()\n");

	if (InitCheck() != B_OK)
		return false;

	fState = TERMINATING;

	if (!IsUp()) {
		fState = INITIAL;
		CloseModem();
		DownEvent();
		return true;
	}

	DownStarted();
		// this tells StateMachine that DownEvent() does not mean we lost connection

	// worker_thread will notice that we are terminating (IsUp() == false)
	// ConnectionLost() will be called so we can terminate the connection there.
	int32 tmp;
	wait_for_thread(fWorkerThread, &tmp);

	DownEvent();

	return true;
}


void
ModemDevice::SetSpeed(uint32 bps)
{
	fInputTransferRate = bps / 8;
	fOutputTransferRate = (fInputTransferRate * 60) / 100;
		// 60% of input transfer rate
}


uint32
ModemDevice::InputTransferRate() const
{
	return fInputTransferRate;
}


uint32
ModemDevice::OutputTransferRate() const
{
	return fOutputTransferRate;
}


uint32
ModemDevice::CountOutputBytes() const
{
	return fOutputBytes;
}


void
ModemDevice::OpenModem()
{
	if (Handle() >= 0)
		return;

	fHandle = open(PortName(), O_RDWR);

	// init port
	struct termios options;
	if (ioctl(fHandle, TCGETA, &options) != B_OK) {
		ERROR("ModemDevice: Could not retrieve port options!\n");
		return;
	}

	// adjust options
	options.c_cflag &= ~CBAUD;
	options.c_cflag |= B115200;
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;
	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 10;

	// set new options
	if (ioctl(fHandle, TCSETA, &options) != B_OK) {
		ERROR("ModemDevice: Could not init port!\n");
		return;
	}
}


void
ModemDevice::CloseModem()
{
	if (Handle() >= 0)
		close(Handle());

	fHandle = -1;
}


void
ModemDevice::FinishedDialing()
{
	fOutputBytes = 0;
	fState = OPENED;
	UpEvent();
}


void
ModemDevice::FailedDialing()
{
	fWorkerThread = -1;
	fState = INITIAL;
	CloseModem();
	UpFailedEvent();
}


void
ModemDevice::ConnectionLost()
{
	// switch to command mode and disconnect
	fWorkerThread = -1;
	fOutputBytes = 0;
	snooze(ESCAPE_DELAY);
	if (write(Handle(), ESCAPE_SEQUENCE, strlen(ESCAPE_SEQUENCE)) < 0)
		return;
	snooze(ESCAPE_DELAY);

	modem_put_line(Handle(), AT_HANG_UP, strlen(AT_HANG_UP));
	CloseModem();
}


status_t
ModemDevice::Send(net_buffer *packet, uint16 protocolNumber)
{
#if DEBUG
	TRACE("ModemDevice: Send()\n");
	dump_packet(packet);
#endif

	if (!packet)
		return B_ERROR;
	else if (InitCheck() != B_OK || protocolNumber != 0) {
		gBufferModule->free(packet);
		return B_ERROR;
	} else if (!IsUp()) {
		gBufferModule->free(packet);
		return PPP_NO_CONNECTION;
	}

	uint8 buffer[2 * (MODEM_MTU + PACKET_OVERHEAD)];

	// add header
	if (fACFC->LocalState() != ACFC_ACCEPTED) {
		NetBufferPrepend<uint8> bufferHeader(packet, 2);
		uint8* data = bufferHeader.operator->();
		data[0] = ALL_STATIONS;
		data[1] = UI;
	}

	int32 position = 0;
	int32 length = packet->size;
	int32 offset = (fACFC->LocalState() != ACFC_ACCEPTED) ? 2 : 0;
	uint8* data;
	if (gBufferModule->direct_access(packet, offset, length, (void**)&data) != B_OK) {
		ERROR("ModemDevice: Failed to access buffer!\n");
		return B_ERROR;
	}

	// add FCS
	uint16 fcs = 0xffff;
	fcs = pppfcs16(fcs, data, length);
	fcs ^= 0xffff;
	data[length++] = fcs & 0x00ff;
	data[length++] = (fcs & 0xff00) >> 8;

	// encode packet
	buffer[position++] = FLAG_SEQUENCE;
		// mark beginning of packet
	for (int32 index = 0; index < length; index++) {
		if (data[index] < 0x20 || data[index] == FLAG_SEQUENCE
				|| data[index] == CONTROL_ESCAPE) {
			buffer[position++] = CONTROL_ESCAPE;
			buffer[position++] = data[index] ^ 0x20;
		} else
			buffer[position++] = data[index];
	}
	buffer[position++] = FLAG_SEQUENCE;
		// mark end of packet

	gBufferModule->free(packet);
	data = NULL;

	// send to modem
	atomic_add((int32*) &fOutputBytes, position);
	if (write(Handle(), buffer, position) < 0)
		return PPP_NO_CONNECTION;
	atomic_add((int32*) &fOutputBytes, -position);

	return B_OK;
}


status_t
ModemDevice::DataReceived(uint8 *buffer, uint32 length)
{
	// TODO: report corrupted packets to KPPPInterface
	if (length < 3)
		return B_ERROR;

	// check FCS
	uint16 fcs = 0xffff;
	fcs = pppfcs16(fcs, buffer, length - 2);
	fcs ^= 0xffff;
	if (buffer[length - 2] != (fcs & 0x00ff)
		|| buffer[length - 1] != (fcs & 0xff00) >> 8) {
		ERROR("ModemDevice: Incorrect FCS!\n");
		return B_ERROR;
	}

	if (buffer[0] == ALL_STATIONS && buffer[1] == UI)
		buffer += 2;

	net_buffer* packet = gBufferModule->create(length - 2);
	if (gBufferModule->write(packet, 0, buffer, length - 2) != B_OK) {
		ERROR("ModemDevice: Failed to write into packet!\n");
		return B_ERROR;
	}

	return Receive(packet);
}


status_t
ModemDevice::Receive(net_buffer *packet, uint16 protocolNumber)
{
	// we do not need to lock because only the worker_thread calls this method

	if (!packet)
		return B_ERROR;
	else if (InitCheck() != B_OK || !IsUp()) {
		gBufferModule->free(packet);
		return B_ERROR;
	}

	return Interface().ReceiveFromDevice(packet);
}
