#ifndef _K_PPP_DEVICE__H
#define _K_PPP_DEVICE__H

#include "KPPPInterface.h"


class PPPDevice {
	public:
		PPPDevice(const char *fName, PPPInterface *interface, driver_parameter *settings);
		virtual ~PPPDevice();
		
		virtual status_t InitCheck() const = 0;
		
		const char *Name() const
			{ return fName; }
		
		PPPInterface *Interface() const
			{ return fInterface; }
		driver_parameter *Settings()
			{ return fSettings; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		
		virtual bool SetMTU(uint32 mtu) = 0;
		uint32 MTU() const
			{ return fMTU; }
		virtual bool LockMTU();
		virtual bool UnlockMTU();
		virtual uint32 PreferredMTU() const = 0;
		
		virtual void Up() = 0;
		virtual void Down() = 0;
		virtual void Listen() = 0;
		bool IsUp() const
			{ return fIsUp; }
		
		virtual status_t Send(mbuf *packet) = 0;

	protected:
		void SetUp(bool isUp);
			// report up/down events

	protected:
		bool fIsUp;

	private:
		char *fName;
		PPPInterface *fInterface;
		driver_parameter *fSettings;
		
		uint32 fMTU;
		int32 fMTULock;
};


#endif
