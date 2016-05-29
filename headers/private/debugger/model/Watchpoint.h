/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef WATCHPOINT_H
#define WATCHPOINT_H


#include <ObjectList.h>
#include <Referenceable.h>

#include "types/Types.h"


class Watchpoint : public BReferenceable {
public:
								Watchpoint(target_addr_t address, uint32 type,
									int32 length);
								~Watchpoint();

			target_addr_t		Address() const		{ return fAddress; }
			uint32				Type() const		{ return fType; }
			int32				Length() const	{ return fLength; }

			bool				IsInstalled() const	{ return fInstalled; }
			void				SetInstalled(bool installed);

			bool				IsEnabled() const	{ return fEnabled; }
			void				SetEnabled(bool enabled);
									// WatchpointManager only

			bool				ShouldBeInstalled() const
									{ return fEnabled && !fInstalled; }

			bool				Contains(target_addr_t address) const;

	static	int					CompareWatchpoints(const Watchpoint* a,
									const Watchpoint* b);
	static	int					CompareAddressWatchpoint(
									const target_addr_t* address,
									const Watchpoint* watchpoint);

private:
			target_addr_t		fAddress;
			uint32				fType;
			int32				fLength;

			bool				fInstalled;
			bool				fEnabled;
};


typedef BObjectList<Watchpoint> WatchpointList;


#endif	// WATCHPOINT_H
