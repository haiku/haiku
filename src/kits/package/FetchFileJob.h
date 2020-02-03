/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Copyright 2013, Rene Gollent <rene@gollent.com>
 * Copyright 2015, Axel Dörfler <axeld@pinc-software.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PRIVATE__FETCH_FILE_JOB_H_
#define _PACKAGE__PRIVATE__FETCH_FILE_JOB_H_


#include <Entry.h>
#include <File.h>
#include <String.h>

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#	include <UrlProtocolListener.h>
#endif

#include <package/Job.h>


namespace BPackageKit {

namespace BPrivate {


#ifdef HAIKU_TARGET_PLATFORM_HAIKU
class FetchFileJob : public BJob, public BUrlProtocolListener {
#else // HAIKU_TARGET_PLATFORM_HAIKU
class FetchFileJob : public BJob {
#endif // HAIKU_TARGET_PLATFORM_HAIKU

	typedef	BJob				inherited;

public:
								FetchFileJob(const BContext& context,
									const BString& title,
									const BString& fileURL,
									const BEntry& targetEntry);
	virtual						~FetchFileJob();

			float				DownloadProgress() const;
			const char*			DownloadURL() const;
			const char*			DownloadFileName() const;
			off_t				DownloadBytes() const;
			off_t				DownloadTotalBytes() const;

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	virtual void	DataReceived(BUrlRequest*, const char* data,
						off_t position, ssize_t size);
	virtual void	DownloadProgress(BUrlRequest*, ssize_t bytesReceived,
						ssize_t bytesTotal);
	virtual void 	RequestCompleted(BUrlRequest* request, bool success);
#endif

protected:
	virtual	status_t			Execute();
	virtual	void				Cleanup(status_t jobResult);

private:
			BString				fFileURL;
			BEntry				fTargetEntry;
			BFile				fTargetFile;
			status_t			fError;
			float				fDownloadProgress;
			off_t				fBytes;
			off_t				fTotalBytes;
};


}	// namespace BPrivate

}	// namespace BPackageKit


#endif // _PACKAGE__PRIVATE__FETCH_FILE_JOB_H_
