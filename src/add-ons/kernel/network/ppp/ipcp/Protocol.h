/*
 * Copyright 2003-2006, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef PROTOCOL__H
#define PROTOCOL__H

#include <driver_settings.h>

#include <KPPPProtocol.h>

#include <arpa/inet.h>
#include <net_datalink.h>
#include <net_datalink_protocol.h>
#include <net/route.h>


#define IPCP_PROTOCOL	0x8021
#define IP_PROTOCOL		0x0021


typedef struct ip_item {
	uint8 type;
	uint8 length;
	in_addr_t address;
} _PACKED ip_item;


enum ipcp_configure_item_codes {
	IPCP_ADDRESSES = 1,
	IPCP_COMPRESSION_PROTOCOL = 2,
	IPCP_ADDRESS = 3,
	IPCP_PRIMARY_DNS = 129,
	IPCP_SECONDARY_DNS = 131
};


typedef struct ipcp_requests {
	// let peer suggest value if ours equals 0.0.0.0
	in_addr_t address;
	in_addr_t netmask;
		// if equal 0.0.0.0 it will be chosen automatically
	in_addr_t primaryDNS;
	in_addr_t secondaryDNS;
} ipcp_requests;


// the values that were negotiated
typedef struct ipcp_configuration {
	in_addr_t address;
	in_addr_t primaryDNS;
	in_addr_t secondaryDNS;
} ipcp_configuration;


extern struct protosw *gProto[];
	// defined in ipcp.cpp


class IPCP : public KPPPProtocol {
	public:
		IPCP(KPPPInterface& interface, driver_parameter *settings);
		virtual ~IPCP();

		virtual void Uninit();

		ppp_state State() const
			{ return fState; }

		virtual status_t StackControl(uint32 op, void *data);

		virtual bool Up();
		virtual bool Down();

		virtual status_t Send(net_buffer *packet,
			uint16 protocolNumber = IPCP_PROTOCOL);
		virtual status_t Receive(net_buffer *packet, uint16 protocolNumber);
		status_t ReceiveIPPacket(net_buffer *packet, uint16 protocolNumber);
		virtual void Pulse();

	private:
		bool ParseSideRequests(const driver_parameter *requests, ppp_side side);
		void UpdateAddresses();
		void RemoveRoutes();

		// for state machine
		void NewState(ppp_state next);
		uint8 NextID();
			// return the next id for IPCP packets

		// events
		void TOGoodEvent();
		void TOBadEvent();
		void RCREvent(net_buffer *packet);
		void RCRGoodEvent(net_buffer *packet);
		void RCRBadEvent(net_buffer *nak, net_buffer *reject);
		void RCAEvent(net_buffer *packet);
		void RCNEvent(net_buffer *packet);
		void RTREvent(net_buffer *packet);
		void RTAEvent(net_buffer *packet);
		void RUCEvent(net_buffer *packet);
		void RXJBadEvent(net_buffer *packet);

		// actions
		void IllegalEvent(ppp_event event);
		void ReportUpFailedEvent();
		void ReportUpEvent();
		void ReportDownEvent();
		void InitializeRestartCount();
		void ResetRestartCount();
		bool SendConfigureRequest();
		bool SendConfigureAck(net_buffer *packet);
		bool SendConfigureNak(net_buffer *packet);
		bool SendTerminateRequest();
		bool SendTerminateAck(net_buffer *request = NULL);
		bool SendCodeReject(net_buffer *packet);

	private:
		ipcp_configuration fLocalConfiguration, fPeerConfiguration;
		ipcp_requests fLocalRequests, fPeerRequests;

		// default route
		struct sockaddr_in fGateway;
		net_route *fDefaultRoute;

		// used for local requests
		bool fRequestPrimaryDNS, fRequestSecondaryDNS;

		// for state machine
		ppp_state fState;
		int32 fID;

		// counters and timers
		int32 fMaxRequest, fMaxTerminate, fMaxNak;
		int32 fRequestCounter, fTerminateCounter, fNakCounter;
		uint8 fRequestID, fTerminateID;
			// the ID we used for the last configure/terminate request
		bigtime_t fNextTimeout;
};


#endif
