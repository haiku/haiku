//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _PPP_INTERFACE_LISTENER__H
#define _PPP_INTERFACE_LISTENER__H

#include <PPPManager.h>
#include <Locker.h>

class BHandler;


#define PPP_REPORT_MESSAGE	'P3RM'
	// the what field of report messages


class PPPInterfaceListener {
	public:
		PPPInterfaceListener(BHandler *target);
		PPPInterfaceListener(const PPPInterfaceListener& copy);
		~PPPInterfaceListener();
		
		status_t InitCheck() const;
		
		BHandler *Target() const
			{ return fTarget; }
		void SetTarget(BHandler *target);
		
		const PPPManager *Manager() const
			{ return &fManager; }
		
		PPPInterfaceListener& operator= (const PPPInterfaceListener& copy)
			{ SetTarget(copy.Target()); return *this; }
				// all interface listeners have the same number of interfaces

	private:
		void Construct();

	private:
		BHandler *fTarget;
		thread_id fReportThread;
		
		PPPManager fManager;
		BLocker fLock;
};


#endif
