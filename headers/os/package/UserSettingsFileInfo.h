/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__USER_SETTINGS_FILE_INFO_H_
#define _PACKAGE__USER_SETTINGS_FILE_INFO_H_


#include <String.h>


namespace BPackageKit {


namespace BHPKG {
	struct BUserSettingsFileInfoData;
}


class BUserSettingsFileInfo {
public:
								BUserSettingsFileInfo();
								BUserSettingsFileInfo(
									const BHPKG::BUserSettingsFileInfoData&
										infoData);
								BUserSettingsFileInfo(const BString& path,
									const BString& templatePath = BString());
								BUserSettingsFileInfo(const BString& path,
									bool isDirectory);
								~BUserSettingsFileInfo();

			status_t			InitCheck() const;

			const BString&		Path() const;
			const BString&		TemplatePath() const;
			bool				IsDirectory() const;

			void				SetTo(const BString& path,
									const BString& templatePath = BString());
			void				SetTo(const BString& path,
									bool isDirectory);

private:
			BString				fPath;
			BString				fTemplatePath;
			bool				fIsDirectory;
};


}	// namespace BPackageKit


#endif	// _PACKAGE__USER_SETTINGS_FILE_INFO_H_
