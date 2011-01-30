/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__ERROR_OUTPUT_H_
#define _PACKAGE__HPKG__ERROR_OUTPUT_H_


#include <stdarg.h>

#include <SupportDefs.h>


namespace BPackageKit {

namespace BHPKG {


class BErrorOutput {
public:
	virtual						~BErrorOutput();

	virtual	void				PrintErrorVarArgs(const char* format,
									va_list args) = 0;
			void				PrintError(const char* format, ...);
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PACKAGE_OUTPUT_H_
