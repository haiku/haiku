/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef INDEX_SERVER_ADD_ON_H
#define INDEX_SERVER_ADD_ON_H


#include <Autolock.h>
#include <Entry.h>
#include <image.h>
#include <ObjectList.h>
#include <String.h>
#include <Volume.h>

#include "Referenceable.h"


class analyser_settings {
public:
						analyser_settings();

	bool				catchUpEnabled;
	//! the volume is scanned form 0 to syncPosition, from
	//! syncPosition to watchingStart the volume is not scanned
	bigtime_t			syncPosition;
	bigtime_t			watchingStart;
	bigtime_t			watchingPosition;
};


class FileAnalyser;


/*! Thread safe class to sync settings between different FileAnalyser. For
example the watcher analyser and the catch up analyser. Because the lock
overhead use it only when necessary or use a cached analyser_settings if
possible. */
class AnalyserSettings : public BReferenceable {
public:
								AnalyserSettings(const BString& name,
									const BVolume& volume);

			const BString&		Name() { return fName; }
			const BVolume&		Volume() { return fVolume; }

			bool				ReadSettings();
			bool				WriteSettings();

			analyser_settings	RawSettings();

			// settings
			void				SetCatchUpEnabled(bool enabled);
			void				SetSyncPosition(bigtime_t time);
			void				SetWatchingStart(bigtime_t time);
			void				SetWatchingPosition(bigtime_t time);

			bool				CatchUpEnabled();
			bigtime_t			SyncPosition();
			bigtime_t			WatchingStart();
			bigtime_t			WatchingPosition();
private:
			BString				fName;
			BVolume				fVolume;

			BLocker				fSettingsLock;
			analyser_settings	fAnalyserSettings;
};


class FileAnalyser {
public:
								FileAnalyser(const BString& name,
									const BVolume& volume);
	virtual						~FileAnalyser() {}

			void				SetSettings(AnalyserSettings* settings);
			AnalyserSettings*	Settings() const;
			const analyser_settings&	CachedSettings() const;
			void				UpdateSettingsCache();

			const BString&		Name() const { return fName; }
			const BVolume&		Volume() const { return fVolume; }

	virtual status_t			InitCheck() = 0;

	virtual void				AnalyseEntry(const entry_ref& ref) = 0;
	virtual void				DeleteEntry(const entry_ref& ref) { }
	virtual void				MoveEntry(const entry_ref& oldRef,
									const entry_ref& newRef) { }
	//! If the indexer send a bunch of entry this indicates that the last one
	//! has been arrived.
	virtual void				LastEntry() { }

protected:
			BVolume				fVolume;
			BReference<AnalyserSettings>	fAnalyserSettings;
			analyser_settings	fCachedSettings;
private:
			BString				fName;
};


typedef BObjectList<FileAnalyser> FileAnalyserList;


class IndexServerAddOn {
public:
	IndexServerAddOn(image_id id, const char* name)
		:
		fImageId(id),
		fName(name)
	{
	}

	virtual						~IndexServerAddOn() {}

			image_id			ImageId() { return fImageId; }
			BString				Name() { return fName; }

	virtual FileAnalyser*		CreateFileAnalyser(const BVolume& volume) = 0;

private:
		image_id				fImageId;
		BString					fName;
};


typedef IndexServerAddOn* create_index_server_addon(image_id id,
	const char* name);


#endif
