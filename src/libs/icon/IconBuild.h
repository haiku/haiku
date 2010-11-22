/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ICON_BUILD_H
#define ICON_BUILD_H


#ifdef ICON_O_MATIC
#	define _BEGIN_ICON_NAMESPACE
#	define _END_ICON_NAMESPACE
#	define _ICON_NAMESPACE			::
#	define _USING_ICON_NAMESPACE
#else
#	define _BEGIN_ICON_NAMESPACE	namespace BPrivate { namespace Icon {
#	define _END_ICON_NAMESPACE		} }
#	define _ICON_NAMESPACE			BPrivate::Icon::
#	define _USING_ICON_NAMESPACE	using namespace BPrivate::Icon;
#endif


#endif	// ICON_BUILD_H
