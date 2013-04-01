/*
 * Copyright 2009-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__STANDARD_ERROR_OUTPUT_H_
#define _PACKAGE__HPKG__STANDARD_ERROR_OUTPUT_H_


#include <package/hpkg/ErrorOutput.h>


namespace BPackageKit {

namespace BHPKG {


class BStandardErrorOutput : public BErrorOutput {
public:
	virtual	void				PrintErrorVarArgs(const char* format,
									va_list args);
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__STANDARD_ERROR_OUTPUT_H_
