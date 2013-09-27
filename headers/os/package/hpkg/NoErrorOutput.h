/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__NO_ERROR_OUTPUT_H_
#define _PACKAGE__HPKG__NO_ERROR_OUTPUT_H_


#include <package/hpkg/ErrorOutput.h>


namespace BPackageKit {

namespace BHPKG {


class BNoErrorOutput : public BErrorOutput {
public:
	virtual	void				PrintErrorVarArgs(const char* format,
									va_list args);
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__NO_ERROR_OUTPUT_H_
