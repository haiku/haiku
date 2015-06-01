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

#include <package/Job.h>


namespace BPackageKit {

namespace BPrivate {


class FetchFileJob : public BJob {
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

protected:
	virtual	status_t			Execute();
	virtual	void				Cleanup(status_t jobResult);

private:
	// libcurl callbacks
	static	int 				_TransferCallback(void* _job,
									off_t downloadTotal, off_t downloaded,
									off_t uploadTotal,	off_t uploaded);

	static	size_t				_WriteCallback(void* buffer, size_t size,
									size_t nmemb, void* userp);

private:
			BString				fFileURL;
			BEntry				fTargetEntry;
			BFile				fTargetFile;
			float				fDownloadProgress;
			off_t				fBytes;
			off_t				fTotalBytes;
};


}	// namespace BPrivate

}	// namespace BPackageKit


#endif // _PACKAGE__PRIVATE__FETCH_FILE_JOB_H_
