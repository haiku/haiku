/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "CatchUpManager.h"

#include <vector>

#include <Debug.h>
#include <Query.h>

#include "IndexServer.h"


const uint32 kCatchUp = '&CaU';
const uint32 kCatchUpDone = '&CUD';

const bigtime_t kSecond = 1000000;


CatchUpAnalyser::CatchUpAnalyser(const BVolume& volume, time_t start,
	time_t end, BHandler* manager)
	:
	AnalyserDispatcher("CatchUpAnalyser"),
	fVolume(volume),
	fStart(start),
	fEnd(end),
	fCatchUpManager(manager)
{
	
}


void
CatchUpAnalyser::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kCatchUp:
			_CatchUp();
		break;

		default:
			BLooper::MessageReceived(message);
	}
}


void
CatchUpAnalyser::StartAnalysing()
{
	PostMessage(kCatchUp);
	Run();
}


void
CatchUpAnalyser::AnalyseEntry(const entry_ref& ref)
{
	for (int i = 0; i < fFileAnalyserList.CountItems(); i++) {
		FileAnalyser* analyser = fFileAnalyserList.ItemAt(i);
		const analyser_settings& settings = analyser->CachedSettings();
		if (settings.syncPosition / kSecond >= fStart
			&& settings.watchingStart / kSecond <= fEnd)
			analyser->AnalyseEntry(ref);
	}
}


void
CatchUpAnalyser::_CatchUp()
{
	STRACE("_CatchUp start %i, end %i\n", (int)fStart, (int)fEnd);
	for (int i = 0; i < fFileAnalyserList.CountItems(); i++)
		STRACE("- Analyser %s\n", fFileAnalyserList.ItemAt(i)->Name().String());

	BQuery query;
	query.SetVolume(&fVolume);
	query.PushAttr("last_modified");
	query.PushInt32(fStart);
	query.PushOp(B_GE);
	query.PushAttr("last_modified");
	query.PushInt32(fEnd);
	query.PushOp(B_LE);
	query.PushOp(B_AND);

	query.Fetch();

	std::vector<entry_ref> entryList;
	entry_ref ref;
	while (query.GetNextRef(&ref) == B_OK)
		entryList.push_back(ref);

	printf("CatchUpAnalyser:: entryList.size() %i\n", (int)entryList.size());

	if (entryList.size() == 0)
		return;

	for (uint32 i = 0; i < entryList.size(); i++) {
		if (Stopped())
			return;
		if (i % 100 == 0)
			printf("Catch up: %i/%i\n", (int)i,(int)entryList.size());
		AnalyseEntry(entryList[i]);
	}
	LastEntry();

	_WriteSyncSatus(fEnd * kSecond);
	printf("Catched up.\n");

	BMessenger managerMessenger(fCatchUpManager);
	BMessage msg(kCatchUpDone);
	msg.AddPointer("Analyser", this);
	managerMessenger.SendMessage(&msg);
}


void
CatchUpAnalyser::_WriteSyncSatus(bigtime_t syncTime)
{
	for (int i = 0; i < fFileAnalyserList.CountItems(); i++) {
		AnalyserSettings* settings = fFileAnalyserList.ItemAt(i)->Settings();
		ASSERT(settings);
		settings->SetSyncPosition(syncTime);
		settings->WriteSettings();
	}
	
}


CatchUpManager::CatchUpManager(const BVolume& volume)
	:
	fVolume(volume)
{

}


CatchUpManager::~CatchUpManager()
{
	Stop();

	for (int i = 0; i < fFileAnalyserQueue.CountItems(); i++)
		delete fFileAnalyserQueue.ItemAt(i);
}


void
CatchUpManager::MessageReceived(BMessage *message)
{
	CatchUpAnalyser* analyser;
	switch (message->what) {
		case kCatchUpDone:
			message->AddPointer("Analyser", &analyser);
			fCatchUpAnalyserList.RemoveItem(analyser);
			analyser->PostMessage(B_QUIT_REQUESTED);
		break;

		default:
			BHandler::MessageReceived(message);
	}
}


bool
CatchUpManager::AddAnalyser(const FileAnalyser* analyserOrg)
{
	IndexServer* server = (IndexServer*)be_app;
	FileAnalyser* analyser = server->CreateFileAnalyser(analyserOrg->Name(),
		fVolume);
	if (!analyser)
		return false;
	ASSERT(analyserOrg->Settings());
	analyser->SetSettings(analyserOrg->Settings());

	bool status = fFileAnalyserQueue.AddItem(analyser);
	if (!status)
		delete analyser;
	return status;
}


void
CatchUpManager::RemoveAnalyser(const BString& name)
{
	for (int i = 0; i < fFileAnalyserQueue.CountItems(); i++) {
		FileAnalyser* analyser = fFileAnalyserQueue.ItemAt(i);
		if (analyser->Name() == name) {
			fFileAnalyserQueue.RemoveItem(analyser);
			delete analyser;
		}
	}

	for (int i = 0; i < fCatchUpAnalyserList.CountItems(); i++)
		fCatchUpAnalyserList.ItemAt(i)->RemoveAnalyser(name);
}


bool
CatchUpManager::CatchUp()
{
	STRACE("CatchUpManager::CatchUp()\n");
	bigtime_t startBig = real_time_clock_usecs();
	bigtime_t endBig = 0;
	for (int i = 0; i < fFileAnalyserQueue.CountItems(); i++) {
		FileAnalyser* analyser = fFileAnalyserQueue.ItemAt(i);
 		analyser->UpdateSettingsCache();
		const analyser_settings& settings = analyser->CachedSettings();
		STRACE("%s, %i, %i\n", analyser->Name().String(),
			  (int)settings.syncPosition, (int)settings.watchingStart);
		if (settings.syncPosition < startBig)
			startBig = settings.syncPosition;
		if (settings.watchingStart > endBig)
			endBig = settings.watchingStart;
	}

	CatchUpAnalyser* catchUpAnalyser = new CatchUpAnalyser(fVolume,
		startBig / kSecond, endBig / kSecond, this);
	if (!catchUpAnalyser)
		return false;
	if (!fCatchUpAnalyserList.AddItem(catchUpAnalyser)) {
		delete catchUpAnalyser;
		return false;
	}

	for (int i = 0; i < fFileAnalyserQueue.CountItems(); i++) {
		FileAnalyser* analyser = fFileAnalyserQueue.ItemAt(i);
		// if AddAnalyser fails at least don't leak
		if (!catchUpAnalyser->AddAnalyser(analyser))
			delete analyser;
			
	}
	fFileAnalyserQueue.MakeEmpty();

	catchUpAnalyser->StartAnalysing();
	return true;
}


void
CatchUpManager::Stop()
{
	for (int i = 0; i < fCatchUpAnalyserList.CountItems(); i++) {
		CatchUpAnalyser* catchUpAnalyser = fCatchUpAnalyserList.ItemAt(i);
		catchUpAnalyser->Stop();
		catchUpAnalyser->PostMessage(B_QUIT_REQUESTED);
	}
	fCatchUpAnalyserList.MakeEmpty();
}
