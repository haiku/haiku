/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_WRITING_UTILS_H
#define PACKAGE_WRITING_UTILS_H


#include <SupportDefs.h>

#include <package/hpkg/PackageWriter.h>


using BPackageKit::BHPKG::BPackageWriter;
using BPackageKit::BHPKG::BPackageWriterListener;


status_t	add_current_directory_entries(BPackageWriter& packageWriter,
				BPackageWriterListener& listener, bool skipPackageInfo);


#endif	// PACKAGE_WRITING_UTILS_H
