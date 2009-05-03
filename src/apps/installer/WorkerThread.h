/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2005, Jérôme DUVAL.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <Looper.h>
#include <Messenger.h>
#include <Partition.h>
#include <Volume.h>

class BList;
class BLocker;
class BMenu;
class InstallerWindow;

class WorkerThread : public BLooper {
public:
								WorkerThread(InstallerWindow* window);

	virtual	void				MessageReceived(BMessage* message);

			void				ScanDisksPartitions(BMenu* srcMenu,
									BMenu* dstMenu);

			void				SetPackagesList(BList* list);
			void				SetSpaceRequired(off_t bytes)
									{ fSpaceRequired = bytes; };

			bool				Cancel();
			void				SetLock(BLocker* lock)
									{ fCancelLock = lock; }

			void				StartInstall();
			void				WriteBootSector(BMenu* dstMenu);

private:
			void				_LaunchInitScript(BPath& path);
			void				_LaunchFinishScript(BPath& path);

			void				_PerformInstall(BMenu* srcMenu, BMenu* dstMenu);

			void				_SetStatusMessage(const char* status);

			InstallerWindow*	fWindow;
			BDiskDeviceRoster	fDDRoster;
			BList*				fPackages;
			off_t				fSpaceRequired;
			BLocker*			fCancelLock;
};

#endif // WORKER_THREAD_H
