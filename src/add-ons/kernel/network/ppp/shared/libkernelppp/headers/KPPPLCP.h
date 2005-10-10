/*
 * Copyright 2003-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

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


//!	LCP packet header structure.
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
		//!	Returns the KPPPStateMachine of the interface that owns this protocol.
		KPPPStateMachine& StateMachine() const
			{ return fStateMachine; }
		
		bool AddOptionHandler(KPPPOptionHandler *handler);
		bool RemoveOptionHandler(KPPPOptionHandler *handler);
		//!	Returns number of registered KPPPOptionHandler objects.
		int32 CountOptionHandlers() const
			{ return fOptionHandlers.CountItems(); }
		KPPPOptionHandler *OptionHandlerAt(int32 index) const;
		KPPPOptionHandler *OptionHandlerFor(uint8 type, int32 *start = NULL) const;
		
		bool AddLCPExtension(KPPPLCPExtension *extension);
		bool RemoveLCPExtension(KPPPLCPExtension *extension);
		//!	Returns number of registered KPPPLCPExtension objects.
		int32 CountLCPExtensions() const
			{ return fLCPExtensions.CountItems(); }
		KPPPLCPExtension *LCPExtensionAt(int32 index) const;
		KPPPLCPExtension *LCPExtensionFor(uint8 code, int32 *start = NULL) const;
		
		/*!	\brief Sets the target protocol handler for outgoing LCP packets.
			
			This may be used for filtering or routing LCP packets. Multilink
			protocols might need this method. \n
			If \a target != \c NULL all packets will be passed to the given protocol
			instead of the interface/device.
		*/
		void SetTarget(KPPPProtocol *target)
			{ fTarget = target; }
		//!	Returns the LCP packet handler or \c NULL.
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
