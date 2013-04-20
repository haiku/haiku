/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__DOWNLOAD_FILE_REQUEST_H_
#define _PACKAGE__DOWNLOAD_FILE_REQUEST_H_


#include <Entry.h>
#include <String.h>

#include <package/Context.h>
#include <package/Request.h>


namespace BPackageKit {


class DownloadFileRequest : public BRequest {
	typedef	BRequest				inherited;

public:
								DownloadFileRequest(const BContext& context,
									const BString& fileURL,
									const BEntry& targetEntry,
									const BString& checksum = BString());
	virtual						~DownloadFileRequest();

	virtual	status_t			CreateInitialJobs();

private:
			BString				fFileURL;
			BEntry				fTargetEntry;
			BString				fChecksum;
};


}	// namespace BPackageKit


#endif // _PACKAGE__DOWNLOAD_FILE_REQUEST_H_
