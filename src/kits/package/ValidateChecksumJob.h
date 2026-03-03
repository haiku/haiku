/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PRIVATE__VALIDATE_CHECKSUM_JOB_H_
#define _PACKAGE__PRIVATE__VALIDATE_CHECKSUM_JOB_H_


#include <Directory.h>
#include <Entry.h>
#include <String.h>

#include <package/ChecksumAccessors.h>
#include <package/Job.h>


namespace BPackageKit {

namespace BPrivate {


class ValidateChecksumJob : public BJob {
	typedef	BJob				inherited;

public:
								ValidateChecksumJob(
									const BContext& context,
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


}	// namespace BPrivate

}	// namespace BPackageKit


#endif // _PACKAGE__PRIVATE__VALIDATE_CHECKSUM_JOB_H_
