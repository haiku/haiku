//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _PPP_INTERFACE__H
#define _PPP_INTERFACE__H

#include <KPPPManager.h>


class PPPInterface {
	public:
		PPPInterface(interface_id ID = PPP_UNDEFINED_INTERFACE_ID);
		PPPInterface(const PPPInterface& copy);
		~PPPInterface();
		
		status_t InitCheck() const;
		
		status_t SetTo(interface_id ID);
		interface_id ID() const
			{ return fID; }
		
		status_t Control(uint32 op, void *data, size_t length) const;
		
		bool GetInterfaceInfo(ppp_interface_info_t *info) const;
		
		bool Up() const;
		bool Down() const;
		
		bool EnableReports(ppp_report_type type, thread_id thread,
			int32 flags = PPP_NO_FLAGS) const;
		bool DisableReports(ppp_report_type type, thread_id thread) const;
		
		PPPInterface& operator= (const PPPInterface& copy)
			{ SetTo(copy.ID()); return *this; }
		PPPInterface& operator= (interface_id ID)
			{ SetTo(ID); return *this; }

	private:
		interface_id fID;
		
		int fFD;
		ppp_interface_info_t fInfo;
};


#endif
