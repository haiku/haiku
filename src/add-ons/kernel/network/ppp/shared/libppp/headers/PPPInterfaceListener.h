//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _PPP_INTERFACE_LISTENER__H
#define _PPP_INTERFACE_LISTENER__H

#include <PPPManager.h>
#include <Locker.h>

class BHandler;


#define PPP_REPORT_MESSAGE	'P3RM'
	//!< The what field of PPPInterfaceListener report messages.


class PPPInterfaceListener {
		friend class PPPInterfaceListenerThread;

	public:
		PPPInterfaceListener(BHandler *target);
		PPPInterfaceListener(const PPPInterfaceListener& copy);
		~PPPInterfaceListener();
		
		status_t InitCheck() const;
		
		//!	Returns the target BHandler for the report messages.
		BHandler *Target() const
			{ return fTarget; }
		void SetTarget(BHandler *target);
		
		//!	Returns whether watching an interface.
		bool DoesWatch() const
			{ return fDoesWatch; }
		//!	Returns which interface is being watched or \c PPP_UNDEFINED_INTERFACE_ID.
		ppp_interface_id WatchingInterface() const
			{ return fWatchingInterface; }
		
		//!	Returns the internal PPPManager object used for accessing the PPP stack.
		const PPPManager& Manager() const
			{ return fManager; }
		
		bool WatchInterface(ppp_interface_id ID);
		void WatchAllInterfaces();
		void StopWatchingInterfaces();
		
		//!	Just sets the target to the given listener's target.
		PPPInterfaceListener& operator= (const PPPInterfaceListener& copy)
			{ SetTarget(copy.Target()); return *this; }

	private:
		void Construct();

	private:
		BHandler *fTarget;
		thread_id fReportThread;
		
		bool fDoesWatch;
		ppp_interface_id fWatchingInterface;
		
		PPPManager fManager;
		BLocker fLock;
};


#endif
