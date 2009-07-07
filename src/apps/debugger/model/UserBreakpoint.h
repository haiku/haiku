/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USER_BREAKPOINT_H
#define USER_BREAKPOINT_H

#include <ObjectList.h>
#include <Referenceable.h>
#include <util/DoublyLinkedList.h>

#include "Types.h"


class Breakpoint;
class Function;
class UserBreakpoint;


class UserBreakpointInstance
	: public DoublyLinkedListLinkImpl<UserBreakpointInstance> {
public:
								UserBreakpointInstance(
									UserBreakpoint* userBreakpoint,
									target_addr_t address);

			UserBreakpoint*		GetUserBreakpoint() const
									{ return fUserBreakpoint; }
			target_addr_t		Address() const	{ return fAddress; }

			Breakpoint*			GetBreakpoint() const	{ return fBreakpoint; }
			void				SetBreakpoint(Breakpoint* breakpoint);

private:
			target_addr_t		fAddress;
			UserBreakpoint*		fUserBreakpoint;
			Breakpoint*			fBreakpoint;
};


typedef DoublyLinkedList<UserBreakpointInstance> UserBreakpointInstanceList;


class UserBreakpoint : public Referenceable {
public:
								UserBreakpoint(Function* function);
								~UserBreakpoint();

			Function*			GetFunction() const	{ return fFunction; }

			int32				CountInstances() const;
			UserBreakpointInstance* InstanceAt(int32 index) const;

			// Note: After known to the BreakpointManager, those can only be
			// invoked with the breakpoint manager lock held.
			bool				AddInstance(UserBreakpointInstance* instance);
			void				RemoveInstance(
									UserBreakpointInstance* instance);

			bool				IsValid() const		{ return fValid; }
			void				SetValid(bool valid);
									// BreakpointManager only

			bool				IsEnabled() const	{ return fEnabled; }
			void				SetEnabled(bool enabled);
									// BreakpointManager only

private:
			typedef BObjectList<UserBreakpointInstance> InstanceList;

private:
			Function*			fFunction;
			InstanceList		fInstances;
			bool				fValid;
			bool				fEnabled;
};


#endif	// USER_BREAKPOINT_H
