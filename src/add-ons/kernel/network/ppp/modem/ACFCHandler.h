//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

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


class ACFCHandler : public PPPOptionHandler {
	public:
		ACFCHandler(uint32 options, PPPInterface& interface);
		
		uint32 Options() const
			{ return fOptions; }
		acfc_state LocalState() const
			{ return fLocalState; }
		acfc_state PeerState() const
			{ return fPeerState; }
		
		virtual status_t AddToRequest(PPPConfigurePacket& request);
		virtual status_t ParseNak(const PPPConfigurePacket& nak);
		virtual status_t ParseReject(const PPPConfigurePacket& reject);
		virtual status_t ParseAck(const PPPConfigurePacket& ack);
		
		virtual status_t ParseRequest(const PPPConfigurePacket& request,
			int32 index, PPPConfigurePacket& nak, PPPConfigurePacket& reject);
		virtual status_t SendingAck(const PPPConfigurePacket& ack);
		
		virtual void Reset();

	private:
		uint32 fOptions;
		acfc_state fLocalState, fPeerState;
};


#endif
