/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */
#ifndef COMMON_OPTIONS_H
#define COMMON_OPTIONS_H


#include <SupportDefs.h>


// common options
enum {
	OPTION_DEBUG	= 256,
};


class CommonOptions {
public:
								CommonOptions();
								~CommonOptions();

			int32				DebugLevel() const
									{ return fDebugLevel; }
			void				SetDebugLevel(int level)
									{ fDebugLevel = level; }

			bool				HandleOption(int option);

private:
			int32				fDebugLevel;
};


#endif	// COMMON_OPTIONS_H
