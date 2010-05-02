/*
 * Copyright 2003, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_FILES_MANAGER_H
#define _MEDIA_FILES_MANAGER_H


#include <map>

#include <Entry.h>
#include <File.h>
#include <Locker.h>
#include <MessageRunner.h>
#include <String.h>

#include "DataExchange.h"


#define MEDIA_FILES_MANAGER_SAVE_TIMER 'mmst'


class MediaFilesManager : BLocker {
public:
								MediaFilesManager();
								~MediaFilesManager();

			status_t			SaveState();

			void				Dump();

			area_id				GetTypesArea(int32& count);
			area_id				GetItemsArea(const char* type, int32& count);

			status_t			GetRefFor(const char* type, const char* item,
									entry_ref** _ref);
			status_t			GetAudioGainFor(const char* type,
									const char* item, float* _gain);
			status_t			SetRefFor(const char* type, const char* item,
									const entry_ref& ref);
			status_t			SetAudioGainFor(const char* type,
									const char* item, float gain);
			status_t			InvalidateItem(const char* type,
									const char* item);
			status_t			RemoveItem(const char* type, const char* item);

			void				TimerMessage();

			void				HandleAddSystemBeepEvent(BMessage* message);

private:
			struct item_info {
				item_info() : gain(1.0f) {}

				entry_ref		ref;
				float			gain;
			};

			void				_LaunchTimer();
			status_t			_GetItem(const char* type, const char* item,
									item_info*& info);
			status_t			_SetItem(const char* type, const char* item,
									const entry_ref* ref = NULL,
									const float* gain = NULL);
			status_t			_OpenSettingsFile(BFile& file, int mode);
			status_t			_LoadState();

private:
			typedef std::map<BString, item_info> ItemMap;
			typedef std::map<BString, ItemMap> TypeMap;

			TypeMap				fMap;
			BMessageRunner*		fSaveTimerRunner;
};

#endif // _MEDIA_FILES_MANAGER_H
