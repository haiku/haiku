/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef WATCHPOINT_SETTING_H
#define WATCHPOINT_SETTING_H


#include <String.h>

#include "types/Types.h"

class BMessage;
class Watchpoint;


class WatchpointSetting {
public:
								WatchpointSetting();
								WatchpointSetting(
									const WatchpointSetting& other);
								~WatchpointSetting();

			status_t			SetTo(const Watchpoint& watchpoint,
									bool enabled);
			status_t			SetTo(const BMessage& archive);
			status_t			WriteTo(BMessage& archive) const;

			target_addr_t		Address() const 	{ return fAddress; }
			uint32				Type() const		{ return fType; }
			int32				Length() const		{ return fLength; }

			bool				IsEnabled() const	{ return fEnabled; }

			WatchpointSetting&	operator=(const WatchpointSetting& other);

private:
			target_addr_t		fAddress;
			uint32				fType;
			int32				fLength;
			bool				fEnabled;
};


#endif	// BREAKPOINT_SETTING_H
