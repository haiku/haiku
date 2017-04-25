/*
 * Copyright 2003-2004, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef ACFC_HANDLER__H
#define ACFC_HANDLER__H

#include <KPPPOptionHandler.h>


// ACFC options
enum {
	REQUEST_ACFC = 0x01,
		// try to request ACFC (does not fail if not successful)
	ALLOW_ACFC = 0x02,
		// allow ACFC if other side requests it
	FORCE_ACFC_REQUEST = 0x04,
		// if ACFC request fails the connection attempt will terminate
};

enum acfc_state {
	ACFC_DISABLED,
	ACFC_ACCEPTED,
	ACFC_REJECTED
		// not used for peer state
};


class ACFCHandler : public KPPPOptionHandler {
	public:
		ACFCHandler(uint32 options, KPPPInterface& interface);

		uint32 Options() const
			{ return fOptions; }
		acfc_state LocalState() const
			{ return fLocalState; }
		acfc_state PeerState() const
			{ return fPeerState; }

		virtual status_t AddToRequest(KPPPConfigurePacket& request);
		virtual status_t ParseNak(const KPPPConfigurePacket& nak);
		virtual status_t ParseReject(const KPPPConfigurePacket& reject);
		virtual status_t ParseAck(const KPPPConfigurePacket& ack);

		virtual status_t ParseRequest(const KPPPConfigurePacket& request,
			int32 index, KPPPConfigurePacket& nak, KPPPConfigurePacket& reject);
		virtual status_t SendingAck(const KPPPConfigurePacket& ack);

		virtual void Reset();

	private:
		uint32 fOptions;
		acfc_state fLocalState, fPeerState;
};


#endif
