/*
 * Copyright 2003-2004, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _K_PPP_DEVICE__H
#define _K_PPP_DEVICE__H

#include <KPPPDefs.h>
#include <KPPPLayer.h>

#ifndef _K_PPP_INTERFACE__H
#include <KPPPInterface.h>
#endif


class KPPPDevice : public KPPPLayer {
		friend class KPPPStateMachine;

	protected:
		// KPPPDevice must be subclassed
		KPPPDevice(const char *name, uint32 overhead, KPPPInterface& interface,
			driver_parameter *settings);

	public:
		virtual ~KPPPDevice();
		
		//!	Returns the interface that owns this device.
		KPPPInterface& Interface() const
			{ return fInterface; }
		//!	Returns the device's settings.
		driver_parameter *Settings() const
			{ return fSettings; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		
		//!	Returns the device's MTU.
		uint32 MTU() const
			{ return fMTU; }
		
		/*!	\brief This brings the device up.
			
			ATTENTION: This method must not block! \n
			Call UpStarted() to check if you are allowed to go down. After UpStarted()
			is called the connection attempt may be aborted by calling Down(). \n
			In server mode you should listen for incoming connections.
			On error: \e Either call \c UpFailedEvent() and return \c true \e or
			return \c false only. \e Never call \c UpFailedEvent() and return
			\c false at the same time!
			
			\sa UpStarted()
			\sa UpFailedEvent()
		*/
		virtual bool Up() = 0;
		/*!	\brief Bring the interface down.
			
			Call DownStarted() to check if you are allowed to go down. \n
			The return value of this method is currently ignored.
			
			\sa DownStarted()
		*/
		virtual bool Down() = 0;
		//!	Is the connection established?
		bool IsUp() const
			{ return fConnectionPhase == PPP_ESTABLISHED_PHASE; }
		//!	Is the interface disconnected and ready to connect?
		bool IsDown() const
			{ return fConnectionPhase == PPP_DOWN_PHASE; }
		//!	Is the interface connecting at the moment?
		bool IsGoingUp() const
			{ return fConnectionPhase == PPP_ESTABLISHMENT_PHASE; }
		//!	Is the interface disconnecting at the moment?
		bool IsGoingDown() const
			{ return fConnectionPhase == PPP_TERMINATION_PHASE; }
		
		/*!	\brief Input speed in bytes per second.
			
			The biggest of the two tranfer rates will be set in ifnet. \n
			Should return default value when disconnected.
		*/
		virtual uint32 InputTransferRate() const = 0;
		/*!	\brief Output speed in bytes per second.
			
			The biggest of the two tranfer rates will be set in ifnet. \n
			Should return default value when disconnected.
		*/
		virtual uint32 OutputTransferRate() const = 0;
		
		//!	Number of bytes waiting to be sent (i.e.: waiting in output queue).
		virtual uint32 CountOutputBytes() const = 0;
		
		virtual bool IsAllowedToSend() const;
		
		/*!	\brief Sends a packet.
			
			This should enqueue the packet and return immediately (never block).
			The device is responsible for freeing the packet by calling \c m_freem().
		*/
		virtual status_t Send(struct mbuf *packet, uint16 protocolNumber) = 0;
		virtual status_t Receive(struct mbuf *packet, uint16 protocolNumber);

	protected:
		//!	Use this to set the device's maximum transfer unit (in bytes).
		void SetMTU(uint32 MTU)
			{ fMTU = MTU; }
		
		bool UpStarted();
		bool DownStarted();
		
		// report up/down events
		void UpFailedEvent();
		void UpEvent();
		void DownEvent();

	protected:
		uint32 fMTU;
			// always hold this value up-to-date!

	private:
		char *fName;
		KPPPInterface& fInterface;
		driver_parameter *fSettings;
		
		ppp_phase fConnectionPhase;
};


#endif
