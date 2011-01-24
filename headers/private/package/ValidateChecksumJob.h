/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__PRIVATE__VALIDATE_CHECKSUM_JOB_H_
#define _HAIKU__PACKAGE__PRIVATE__VALIDATE_CHECKSUM_JOB_H_


#include <Directory.h>
#include <Entry.h>
#include <String.h>

#include <package/ChecksumAccessors.h>
#include <package/Job.h>


namespace Haiku {

namespace Package {

namespace Private {


class ValidateChecksumJob : public Job {
	typedef	Job					inherited;

public:
								ValidateChecksumJob(
									const Context& context,
									const BString& title,
									ChecksumAccessor* expectedChecksumAccessor,
									ChecksumAccessor* realChecksumAccessor,
									bool failIfChecksumsDontMatch = true);
	virtual						~ValidateChecksumJob();

			bool				ChecksumsMatch() const;

protected:
	virtual	status_t			Execute();

private:
			ChecksumAccessor*	fExpectedChecksumAccessor;
			ChecksumAccessor*	fRealChecksumAccessor;
			bool				fFailIfChecksumsDontMatch;

			bool				fChecksumsMatch;
};


}	// namespace Private

}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__PRIVATE__VALIDATE_CHECKSUM_JOB_H_
