#ifndef _K_PPP_LCP__H
#define _K_PPP_LCP__H

#include "KPPPProtocol.h"


typedef struct lcp_packet {
	uint8 code;
	uint8 id;
	uint16 length;
	int8 data[0];
} lcp_packet;


class PPPLCP : public PPPProtocol {
		friend class PPPInterface;

	private:
		// may only be constructed/destructed by PPPInterface
		PPPLCP(PPPInterface& interface);
		~PPPLCP();
		
		// copies are not allowed!
		PPPLCP(const PPPLCP& copy);
		PPPLCP& operator= (const PPPLCP& copy);

	public:
		

	private:
		
};


#endif
