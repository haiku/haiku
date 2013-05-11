/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VERSION_H
#define VERSION_H


#include <package/PackageResolvableOperator.h>
#include <SupportDefs.h>

#include "String.h"


using namespace BPackageKit;


class Version {
public:
								Version();
								~Version();

			status_t			Init(const char* major, const char* minor,
									const char* micro, const char* preRelease,
									uint32 revision);

	static	status_t			Create(const char* major, const char* minor,
									const char* micro, const char* preRelease,
									uint32 revision, Version*& _version);

			int					Compare(const Version& other) const;
			bool				Compare(BPackageResolvableOperator op,
									const Version& other) const;

			size_t				ToString(char* buffer, size_t bufferSize) const;
									// returns how big the buffer should have
									// been (excluding the terminating null)

private:
			String				fMajor;
			String				fMinor;
			String				fMicro;
			String				fPreRelease;
			uint32				fRevision;
};


#endif	// VERSION_H
