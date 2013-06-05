/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__GLOBAL_WRITABLE_FILE_INFO_H_
#define _PACKAGE__GLOBAL_WRITABLE_FILE_INFO_H_


#include <package/WritableFileUpdateType.h>
#include <String.h>


namespace BPackageKit {


namespace BHPKG {
	struct BGlobalWritableFileInfoData;
}


class BGlobalWritableFileInfo {
public:
								BGlobalWritableFileInfo();
								BGlobalWritableFileInfo(
									const BHPKG::BGlobalWritableFileInfoData&
										infoData);
								BGlobalWritableFileInfo(const BString& path,
									BWritableFileUpdateType updateType,
									bool isDirectory);
								~BGlobalWritableFileInfo();

			status_t			InitCheck() const;

			const BString&		Path() const;
			bool				IsIncluded() const;
			BWritableFileUpdateType UpdateType() const;
			bool				IsDirectory() const;

			void				SetTo(const BString& path,
									BWritableFileUpdateType updateType,
									bool isDirectory);

private:
			BString				fPath;
			BWritableFileUpdateType fUpdateType;
			bool				fIsDirectory;
};


}	// namespace BPackageKit


#endif	// _PACKAGE__GLOBAL_WRITABLE_FILE_INFO_H_
