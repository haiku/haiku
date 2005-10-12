/*
 * Copyright 2003-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _PPP_INTERFACE__H
#define _PPP_INTERFACE__H

#include <KPPPManager.h>
#include <String.h>

class BEntry;


class PPPInterface {
	public:
		PPPInterface(ppp_interface_id ID = PPP_UNDEFINED_INTERFACE_ID);
		PPPInterface(const PPPInterface& copy);
		~PPPInterface();
		
		status_t InitCheck() const;
		
		//!	Returns the name of the interface description file.
		const char *Name() const
			{ return fName.String(); }
		
		status_t SetTo(ppp_interface_id ID);
		//!	Returns the unique id of this interface.
		ppp_interface_id ID() const
			{ return fID; }
		
		status_t Control(uint32 op, void *data, size_t length) const;
		
		bool SetUsername(const char *username) const;
		bool SetPassword(const char *password) const;
		bool SetAskBeforeConnecting(bool askBeforeConnecting) const;
		
		status_t GetSettingsEntry(BEntry *entry) const;
		bool GetInterfaceInfo(ppp_interface_info_t *info) const;
		bool GetStatistics(ppp_statistics *statistics) const;
		bool HasSettings(const driver_settings *settings) const;
		
		bool Up() const;
		bool Down() const;
		
		bool EnableReports(ppp_report_type type, thread_id thread,
			int32 flags = PPP_NO_FLAGS) const;
		bool DisableReports(ppp_report_type type, thread_id thread) const;
		
		//!	Same as \c SetTo(copy.ID());
		PPPInterface& operator= (const PPPInterface& copy)
			{ SetTo(copy.ID()); return *this; }
		//!	Same as \c SetTo(ID);
		PPPInterface& operator= (ppp_interface_id ID)
			{ SetTo(ID); return *this; }

	private:
		ppp_interface_id fID;
		int fFD;
		BString fName;
};


#endif
