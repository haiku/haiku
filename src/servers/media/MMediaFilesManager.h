/* 
 * Copyright 2003, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Entry.h>
#include <File.h>
#include <Locker.h>
#include <MessageRunner.h>
#include <String.h>
#include "TMap.h"
#include "DataExchange.h"

#define MMEDIAFILESMANAGER_SAVE_TIMER 'mmst'

class MMediaFilesManager
{
public:
	MMediaFilesManager();
	~MMediaFilesManager();

	status_t LoadState();
	status_t SaveState();

	void Dump();
	
	status_t RewindTypes(
				BString ***types, 
				int32 *count);
	status_t RewindRefs(
				const char * type,
				BString ***items, 
				int32 *count);
	status_t GetRefFor(
				const char * type,
				const char * item,
				entry_ref ** out_ref);
	status_t SetRefFor(
				const char * type,
				const char * item,
				const entry_ref & ref,
				bool save = true);
	status_t RemoveRefFor(		//	This might better be called "ClearRefFor"
				const char * type,		//	but it's too late now...
				const char * item,
				const entry_ref & ref);

	status_t RemoveItem(		//	new in 4.1, removes the whole item.
				const char * type,
				const char * item);

	void TimerMessage();

private:
	static int32 ReadPascalString(BFile &file, char **str);
	static int32 WritePascalString(BFile &file, const char *str);
	void LaunchTimer();
private:
	BLocker *fLocker;
	
	Map<BString, Map<BString, entry_ref> > * fRegistryMap;
	
	uint32 header[3];
	
	BMessageRunner *fRunner;
};
