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
	// the what field of report messages


class PPPInterfaceListener {
		friend class PPPInterfaceListenerThread;

	public:
		PPPInterfaceListener(BHandler *target);
		PPPInterfaceListener(const PPPInterfaceListener& copy);
		~PPPInterfaceListener();
		
		status_t InitCheck() const;
		
		BHandler *Target() const
			{ return fTarget; }
		void SetTarget(BHandler *target);
		
		bool DoesWatch() const
			{ return fDoesWatch; }
		ppp_interface_id WatchingInterface() const
			{ return fWatchingInterface; }
		
		const PPPManager& Manager() const
			{ return fManager; }
		
		bool WatchInterface(ppp_interface_id ID);
		void WatchAllInterfaces();
		void StopWatchingInterfaces();
		
		PPPInterfaceListener& operator= (const PPPInterfaceListener& copy)
			{ SetTarget(copy.Target()); return *this; }
				// all interface listeners have the same number of interfaces

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
