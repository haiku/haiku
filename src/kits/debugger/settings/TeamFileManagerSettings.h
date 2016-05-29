/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_FILE_MANAGER_SETTINGS_H
#define TEAM_FILE_MANAGER_SETTINGS_H

#include <Message.h>


class TeamFileManagerSettings {
public:
								TeamFileManagerSettings();
	virtual						~TeamFileManagerSettings();

			TeamFileManagerSettings&
								operator=(
									const TeamFileManagerSettings& other);
									// throws std::bad_alloc;

			const char*			ID() const;
			status_t			SetTo(const BMessage& archive);
			status_t			WriteTo(BMessage& archive) const;

			int32				CountSourceMappings() const;
			status_t			AddSourceMapping(const BString& sourcePath,
									const BString& locatedPath);
			status_t			RemoveSourceMappingAt(int32 index);
			status_t			GetSourceMappingAt(int32 index,
									BString& sourcePath, BString& locatedPath);

	virtual	TeamFileManagerSettings*
								Clone() const;
									// throws std::bad_alloc

private:
	BMessage					fValues;
};


#endif	// TEAM_FILE_MANAGER_SETTINGS_H
