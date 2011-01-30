/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PACKAGE_WRITER_H_
#define _PACKAGE__HPKG__PACKAGE_WRITER_H_


#include <SupportDefs.h>


namespace BPackageKit {

namespace BHPKG {


namespace BPrivate {
	class PackageWriterImpl;
}
using BPrivate::PackageWriterImpl;


class BPackageWriter {
public:
								BPackageWriter();
								~BPackageWriter();

			status_t			Init(const char* fileName);
			status_t			AddEntry(const char* fileName);
			status_t			Finish();

private:
			PackageWriterImpl*	fImpl;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PACKAGE_WRITER_H_
