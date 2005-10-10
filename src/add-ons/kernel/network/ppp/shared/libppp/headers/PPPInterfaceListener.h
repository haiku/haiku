/*
 * Copyright 2003-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _PPP_INTERFACE_LISTENER__H
#define _PPP_INTERFACE_LISTENER__H

#include <PPPManager.h>
#include <Locker.h>

class BHandler;


#define PPP_REPORT_MESSAGE	'P3RM'
	//!< The what field of PPPInterfaceListener report messages.


class PPPInterfaceListener {
	public:
		PPPInterfaceListener(BHandler *target);
		PPPInterfaceListener(const PPPInterfaceListener& copy);
		~PPPInterfaceListener();
		
		status_t InitCheck() const;
		
		//!	Returns the target BHandler for the report messages.
		BHandler *Target() const
			{ return fTarget; }
		void SetTarget(BHandler *target);
		
		//!	Returns whether the listener is watching an interface.
		bool IsWatching() const
			{ return fIsWatching; }
		//!	Returns which interface is being watched or \c PPP_UNDEFINED_INTERFACE_ID.
		ppp_interface_id Interface() const
			{ return fInterface; }
		
		//!	Returns the internal PPPManager object used for accessing the PPP stack.
		const PPPManager& Manager() const
			{ return fManager; }
		
		bool WatchInterface(ppp_interface_id ID);
		void WatchManager();
		void StopWatchingInterface();
		void StopWatchingManager();
		
		//!	Just sets the target to the given listener's target.
		PPPInterfaceListener& operator= (const PPPInterfaceListener& copy)
			{ SetTarget(copy.Target()); return *this; }

	private:
		void Construct();

	private:
		BHandler *fTarget;
		thread_id fReportThread;
		
		bool fIsWatching;
		ppp_interface_id fInterface;
		
		PPPManager fManager;
};


#endif
