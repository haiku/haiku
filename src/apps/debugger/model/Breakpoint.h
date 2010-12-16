/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include "UserBreakpoint.h"


class Image;


class BreakpointClient {
public:
	virtual						~BreakpointClient();
};


class Breakpoint : public BReferenceable {
public:
								Breakpoint(Image* image, target_addr_t address);
								~Breakpoint();

			Image*				GetImage() const	{ return fImage; }
			target_addr_t		Address() const		{ return fAddress; }

			bool				IsInstalled() const	{ return fInstalled; }
			void				SetInstalled(bool installed);

			bool				ShouldBeInstalled() const;
			bool				IsUnused() const;
			bool				HasEnabledUserBreakpoint() const;

			UserBreakpointInstance* FirstUserBreakpoint() const
									{ return fUserBreakpoints.Head(); }
			UserBreakpointInstance* LastUserBreakpoint() const
									{ return fUserBreakpoints.Tail(); }
			const UserBreakpointInstanceList& UserBreakpoints() const
									{ return fUserBreakpoints; }

			void				AddUserBreakpoint(
									UserBreakpointInstance* instance);
			void				RemoveUserBreakpoint(
									UserBreakpointInstance* instance);

			bool				AddClient(BreakpointClient* client);
			void				RemoveClient(BreakpointClient* client);

	static	int					CompareBreakpoints(const Breakpoint* a,
									const Breakpoint* b);
	static	int					CompareAddressBreakpoint(
									const target_addr_t* address,
									const Breakpoint* breakpoint);

private:
			typedef BObjectList<BreakpointClient> ClientList;

private:
			target_addr_t		fAddress;
			Image*				fImage;
			UserBreakpointInstanceList fUserBreakpoints;
			ClientList			fClients;
			bool				fInstalled;
};


#endif	// BREAKPOINT_H
