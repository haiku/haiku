#ifndef _K_PPP_LCP__H
#define _K_PPP_LCP__H

#include "KPPPProtocol.h"


class PPPLCP : public PPPProtocol {
		friend class PPPInterface;

	private:
		// may only be constructed/destructed by PPPInterface
		PPPLCP(PPPInterface &interface);
		~PPPLCP();

	public:
		

	private:
		
};


#endif
