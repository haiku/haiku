/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BREAKPOINT_SETTING_H
#define BREAKPOINT_SETTING_H


#include <String.h>

#include <ObjectList.h>

#include "SourceLocation.h"
#include "Types.h"


class BMessage;
class FunctionID;
class UserBreakpointLocation;


class BreakpointSetting {
public:
								BreakpointSetting();
								BreakpointSetting(
									const BreakpointSetting& other);
								~BreakpointSetting();

			status_t			SetTo(const UserBreakpointLocation& location,
									bool enabled);
			status_t			SetTo(const BMessage& archive);
			status_t			WriteTo(BMessage& archive) const;

			FunctionID*			GetFunctionID() const	{ return fFunctionID; }
			const BString&		SourceFile() const		{ return fSourceFile; }
			SourceLocation		GetSourceLocation() const
									{ return fSourceLocation; }
			target_addr_t		RelativeAddress() const
									{ return fRelativeAddress; }

			bool				IsEnabled() const	{ return fEnabled; }

			BreakpointSetting&	operator=(const BreakpointSetting& other);

private:
			void				_Unset();

private:
			FunctionID*			fFunctionID;
			BString				fSourceFile;
			SourceLocation		fSourceLocation;
			target_addr_t		fRelativeAddress;
			bool				fEnabled;
};


#endif	// BREAKPOINT_SETTING_H
