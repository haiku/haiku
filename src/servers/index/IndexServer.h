/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef INDEX_SERVER_H
#define INDEX_SERVER_H


#include <Application.h>
#include <MessageRunner.h>
#include <VolumeRoster.h>

#include <AddOnMonitorHandler.h>
#include <ObjectList.h>

#include "IndexServerAddOn.h"
#include "VolumeWatcher.h"


#define DEBUG_INDEX_SERVER
#ifdef DEBUG_INDEX_SERVER
#include <stdio.h>
#	define STRACE(x...) printf(x)
#else
#	define STRACE(x...) ;
#endif


class IndexServer;


class VolumeObserverHandler : public BHandler {
public:
								VolumeObserverHandler(IndexServer* indexServer);
			void				MessageReceived(BMessage *message);
private:
			IndexServer*		fIndexServer;
};


class AnalyserMonitorHandler : public AddOnMonitorHandler {
public:
								AnalyserMonitorHandler(
									IndexServer* indexServer);

private:
			void				AddOnEnabled(
									const add_on_entry_info* entryInfo);
			void				AddOnDisabled(
									const add_on_entry_info* entryInfo);

			IndexServer*		fIndexServer;
};


class IndexServer : public BApplication {
public:
								IndexServer();
	virtual						~IndexServer();

	virtual void				ReadyToRun();
	virtual	void				MessageReceived(BMessage *message);

	virtual	bool				QuitRequested();

			void				AddVolume(const BVolume& volume);
			void				RemoveVolume(const BVolume& volume);

			void				RegisterAddOn(entry_ref ref);
			void				UnregisterAddOn(entry_ref ref);

			//! thread safe
			FileAnalyser*		CreateFileAnalyser(const BString& name,
									const BVolume& volume);
private:
			void				_StartWatchingVolumes();
			void				_StopWatchingVolumes();

			void				_SetupVolumeWatcher(VolumeWatcher* watcher);
			FileAnalyser*		_SetupFileAnalyser(IndexServerAddOn* addon,
									const BVolume& volume);
			void				_StartWatchingAddOns();

	inline	IndexServerAddOn*	_FindAddon(const BString& name);

			BVolumeRoster		fVolumeRoster;
			BObjectList<VolumeWatcher>		fVolumeWatcherList;
			BObjectList<IndexServerAddOn>	fAddOnList;

			VolumeObserverHandler	fVolumeObserverHandler;

			AnalyserMonitorHandler	fAddOnMonitorHandler;
			BMessageRunner*			fPulseRunner;
};


#endif
