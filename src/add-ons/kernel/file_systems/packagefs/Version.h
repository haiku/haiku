/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VERSION_H
#define VERSION_H


#include <package/PackageResolvableOperator.h>
#include <SupportDefs.h>


using namespace BPackageKit;


class Version {
public:
								Version();
								~Version();

			status_t			Init(const char* major, const char* minor,
									const char* micro, uint8 release);

	static	status_t			Create(const char* major, const char* minor,
									const char* micro, uint8 release,
									Version*& _version);

			int					Compare(const Version& other) const;
			bool				Compare(BPackageResolvableOperator op,
									const Version& other) const;

			size_t				ToString(char* buffer, size_t bufferSize) const;
									// returns how big the buffer should have
									// been (excluding the terminating null)

private:
			char*				fMajor;
			char*				fMinor;
			char*				fMicro;
			uint8				fRelease;
};


#endif	// VERSION_H
