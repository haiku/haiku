/*
 * Copyright 2003, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <map>

#include <Entry.h>
#include <File.h>
#include <Locker.h>
#include <MessageRunner.h>
#include <String.h>

#include <DataExchange.h>


#define MEDIA_FILES_MANAGER_SAVE_TIMER 'mmst'


class MediaFilesManager : BLocker {
public:
								MediaFilesManager();
								~MediaFilesManager();

			status_t			LoadState();
			status_t			SaveState();

			void				Dump();

			area_id				GetTypesArea(int32& count);
			area_id				GetItemsArea(const char* type, int32& count);

			status_t			GetRefFor(const char* type, const char* item,
									entry_ref** _ref);
			status_t			SetRefFor(const char* type, const char* item,
									const entry_ref& ref, bool save = true);
			status_t			InvalidateRefFor(const char* type,
									const char* item);
			status_t			RemoveItem(const char* type, const char* item);

			void				TimerMessage();

			void				HandleAddSystemBeepEvent(BMessage* message);

private:
			void				_LaunchTimer();

	static	ssize_t				_ReadSettingsString(BFile& file,
									BString& string);
	static	status_t			_WriteSettingsString(BFile& file,
									const char* string);

private:
			// for each type, the map contains a map of item/entry_ref
			typedef std::map<BString, entry_ref> FileMap;
			typedef std::map<BString, FileMap> TypeMap;

			TypeMap				fMap;
			BMessageRunner*		fSaveTimerRunner;
};
