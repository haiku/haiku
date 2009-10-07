/*
 * Copyright 2007-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	AUTO_MOUNTER_H
#define AUTO_MOUNTER_H

#include <Application.h>
#include <DiskDeviceDefs.h>
#include <File.h>
#include <Message.h>

class BPartition;
class BPath;


class AutoMounter : public BApplication {
public:
								AutoMounter();
	virtual						~AutoMounter();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

private:
			enum mount_mode {
				kNoVolumes,
				kOnlyBFSVolumes,
				kAllVolumes,
				kRestorePreviousVolumes
			};

			void				_GetSettings(BMessage* message);
			bool				_ScriptReceived(BMessage* msg, int32 index,
									BMessage* specifier, int32 form,
									const char* property);

			void				_MountVolumes(mount_mode normal,
									mount_mode removable,
									bool initialRescan = false,
									partition_id deviceID = -1);
			void				_MountVolume(const BMessage* message);
			bool				_SuggestForceUnmount(const char* name,
									status_t error);
			void				_ReportUnmountError(const char* name,
									status_t error);
			void				_UnmountAndEjectVolume(BPartition* partition,
									BPath& mountPoint, const char* name);
			void				_UnmountAndEjectVolume(BMessage* message);

			void				_FromMode(mount_mode mode, bool& all,
									bool& bfs, bool& restore);
			mount_mode			_ToMode(bool all, bool bfs,
									bool restore = false);

			void				_UpdateSettingsFromMessage(BMessage* message);
			void				_ReadSettings();
			void				_WriteSettings();

private:
			mount_mode			fNormalMode;
			mount_mode			fRemovableMode;
			bool				fEjectWhenUnmounting;

			BFile				fPrefsFile;
			BMessage			fSettings;
};

#endif // AUTO_MOUNTER_H
