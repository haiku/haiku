/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__USER_SETTINGS_FILE_INFO_H_
#define _PACKAGE__USER_SETTINGS_FILE_INFO_H_


#include <String.h>


namespace BPackageKit {


class BUserSettingsFileInfo {
public:
								BUserSettingsFileInfo();
								BUserSettingsFileInfo(const BString& path,
									const BString& templatePath = BString());
								~BUserSettingsFileInfo();

			status_t			InitCheck() const;

			const BString&		Path() const;
			const BString&		TemplatePath() const;

			void				SetTo(const BString& path,
									const BString& templatePath = BString());

private:
			BString				fPath;
			BString				fTemplatePath;
};


}	// namespace BPackageKit


#endif	// _PACKAGE__USER_SETTINGS_FILE_INFO_H_
