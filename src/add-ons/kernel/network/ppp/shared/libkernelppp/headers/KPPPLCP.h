//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _K_PPP_LCP__H
#define _K_PPP_LCP__H

#include <TemplateList.h>

#ifndef _K_PPP_PROTOCOL__H
#include <KPPPProtocol.h>
#endif

#ifndef _K_PPP_INTERFACE__H
#include <KPPPInterface.h>
#endif

#ifndef _K_PPP_STATE_MACHINE__H
#include <KPPPStateMachine.h>
#endif

class KPPPLCPExtension;
class KPPPOptionHandler;


typedef struct ppp_lcp_packet {
	uint8 code;
	uint8 id;
	uint16 length;
	uint8 data[0];
} ppp_lcp_packet;


class KPPPLCP : public KPPPProtocol {
		friend class KPPPInterface;

	private:
		// may only be constructed/destructed by KPPPInterface
		KPPPLCP(KPPPInterface& interface);
		virtual ~KPPPLCP();
		
		// copies are not allowed!
		KPPPLCP(const KPPPLCP& copy);
		KPPPLCP& operator= (const KPPPLCP& copy);

	public:
		KPPPStateMachine& StateMachine() const
			{ return fStateMachine; }
		
		bool AddOptionHandler(KPPPOptionHandler *handler);
		bool RemoveOptionHandler(KPPPOptionHandler *handler);
		int32 CountOptionHandlers() const
			{ return fOptionHandlers.CountItems(); }
		KPPPOptionHandler *OptionHandlerAt(int32 index) const;
		KPPPOptionHandler *OptionHandlerFor(uint8 type, int32 *start = NULL) const;
		
		bool AddLCPExtension(KPPPLCPExtension *extension);
		bool RemoveLCPExtension(KPPPLCPExtension *extension);
		int32 CountLCPExtensions() const
			{ return fLCPExtensions.CountItems(); }
		KPPPLCPExtension *LCPExtensionAt(int32 index) const;
		KPPPLCPExtension *LCPExtensionFor(uint8 code, int32 *start = NULL) const;
		
		void SetTarget(KPPPProtocol *target)
			{ fTarget = target; }
			// if target != NULL all packtes will be passed to the protocol
			// instead of the interface/device
		KPPPProtocol *Target() const
			{ return fTarget; }
		
		uint32 AdditionalOverhead() const;
			// the overhead caused by the target, the device, and the interface
		
		virtual bool Up();
		virtual bool Down();
		
		virtual status_t Send(struct mbuf *packet,
			uint16 protocolNumber = PPP_LCP_PROTOCOL);
		virtual status_t Receive(struct mbuf *packet, uint16 protocolNumber);
		
		virtual void Pulse();

	private:
		KPPPStateMachine& fStateMachine;
		
		TemplateList<KPPPOptionHandler*> fOptionHandlers;
		TemplateList<KPPPLCPExtension*> fLCPExtensions;
		
		KPPPProtocol *fTarget;
};


#endif
