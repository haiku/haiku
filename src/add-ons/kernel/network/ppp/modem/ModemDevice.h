/*
 * Copyright 2003-2006, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */
#ifndef MODEM_DEVICE__H
#define MODEM_DEVICE__H


#include "Modem.h"

#include <KPPPDevice.h>


enum modem_state {
	INITIAL,
		// the same as IsDown() == true
	TERMINATING,
	DIALING,
	OPENED
		// the same as IsUp() == true
};


class ACFCHandler;

class ModemDevice : public KPPPDevice {
	public:
		ModemDevice(KPPPInterface& interface, driver_parameter *settings);
		virtual ~ModemDevice();

		const char *PortName() const
			{ return fPortName; }
		int32 Handle() const
			{ return fHandle; }
				// returns file handle for modem driver

		const char *InitString() const
			{ return fInitString; }
		const char *DialString() const
			{ return fDialString; }

		virtual status_t InitCheck() const;

		virtual bool Up();
		virtual bool Down();

		void SetSpeed(uint32 bps);
		virtual uint32 InputTransferRate() const;
		virtual uint32 OutputTransferRate() const;
			// this is around 60% of the input transfer rate

		virtual uint32 CountOutputBytes() const;

		void OpenModem();
		void CloseModem();

		// notifications:
		void FinishedDialing();
		void FailedDialing();
		void ConnectionLost();

		virtual status_t Send(net_buffer *packet, uint16 protocolNumber = 0);
		status_t DataReceived(uint8 *buffer, uint32 length);
			// this will put the data into an mbuf and call Receive()
		virtual status_t Receive(net_buffer *packet, uint16 protocolNumber = 0);

	private:
		const char *fPortName, *fInitString, *fDialString;
		int32 fHandle;
			// file handle for modem driver

		thread_id fWorkerThread;

		uint32 fInputTransferRate, fOutputTransferRate;
		uint32 fOutputBytes;

		modem_state fState;

		ACFCHandler *fACFC;
};


#endif
