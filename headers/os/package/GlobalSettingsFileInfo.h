/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__GLOBAL_SETTINGS_FILE_INFO_H_
#define _PACKAGE__GLOBAL_SETTINGS_FILE_INFO_H_


#include <package/SettingsFileUpdateType.h>
#include <String.h>


namespace BPackageKit {


class BGlobalSettingsFileInfo {
public:
								BGlobalSettingsFileInfo();
								BGlobalSettingsFileInfo(const BString& path,
									BSettingsFileUpdateType updateType);
								~BGlobalSettingsFileInfo();

			status_t			InitCheck() const;

			const BString&		Path() const;
			bool				IsIncluded() const;
			BSettingsFileUpdateType UpdateType() const;

			void				SetTo(const BString& path,
									BSettingsFileUpdateType updateType);

private:
			BString				fPath;
			BSettingsFileUpdateType fUpdateType;
};


}	// namespace BPackageKit


#endif	// _PACKAGE__GLOBAL_SETTINGS_FILE_INFO_H_
