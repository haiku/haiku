/*
 * Copyright 2009-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/StandardErrorOutput.h>

#include <stdio.h>


namespace BPackageKit {

namespace BHPKG {


void
BStandardErrorOutput::PrintErrorVarArgs(const char* format, va_list args)
{
	vfprintf(stderr, format, args);
}


}	// namespace BHPKG

}	// namespace BPackageKit
