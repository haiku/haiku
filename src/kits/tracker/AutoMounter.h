/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#ifndef	_AUTO_MOUNTER_H
#define _AUTO_MOUNTER_H

#include <DiskDeviceDefs.h>
#include <File.h>
#include <Looper.h>
#include <Message.h>

class BPartition;
class BPath;

namespace BPrivate {

const uint32 kMountVolume 				= 'mntv';
const uint32 kMountAllNow				= 'mntn';
const uint32 kSetAutomounterParams 		= 'pmst';
const uint32 kVolumeMounted				= 'vmtd';


class AutoMounter : public BLooper {
public:
								AutoMounter();
	virtual						~AutoMounter();

	virtual	bool				QuitRequested();

			void				GetSettings(BMessage* message);

private:
			enum mount_mode {
				kNoVolumes,
				kOnlyBFSVolumes,
				kAllVolumes,
				kRestorePreviousVolumes
			};

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

	virtual	void				MessageReceived(BMessage* message);

private:
			mount_mode			fNormalMode;
			mount_mode			fRemovableMode;

			BFile				fPrefsFile;
			BMessage			fSettings;
};


} // namespace BPrivate

using namespace BPrivate;

#endif	// _AUTO_MOUNTER_H
