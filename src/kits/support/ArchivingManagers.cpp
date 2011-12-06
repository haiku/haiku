/*
 * Copyright (c) 2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alex Wilson (yourpalal2@gmail.com)
 */

#include "ArchivingManagers.h"

#include <syslog.h>
#include <typeinfo>


namespace BPrivate {
namespace Archiving {
	const char* kArchivableField = "_managed_archivable";
	const char* kManagedField = "_managed_archive";
}
}


using namespace BPrivate::Archiving;


BArchiveManager*
BManagerBase::ArchiveManager(const BMessage* archive)
{
	BManagerBase* manager = ManagerPointer(archive);
	if (!manager)
		return NULL;

	if (manager->fType == ARCHIVE_MANAGER)
		return static_cast<BArchiveManager*>(manager);

	debugger("Overlapping managed unarchive/archive sessions.");
	return NULL;
}


BUnarchiveManager*
BManagerBase::UnarchiveManager(const BMessage* archive)
{
	BManagerBase* manager = ManagerPointer(archive);
	if (!manager)
		return NULL;

	if (manager->fType == UNARCHIVE_MANAGER)
		return static_cast<BUnarchiveManager*>(manager);

	debugger("More calls to BUnarchiver::PrepareArchive()"
		" than BUnarchivers created.");

	return NULL;
}


// #pragma mark -


struct BArchiveManager::ArchiveInfo {
	ArchiveInfo()
		:
		token(-1),
		archive(NULL)
	{
	}


	~ArchiveInfo()
	{
		delete archive;
	}


	int32		token;
	BMessage*	archive;
};


BArchiveManager::BArchiveManager(const BArchiver* creator)
	:
	BManagerBase(creator->ArchiveMessage(), BManagerBase::ARCHIVE_MANAGER),
	fTokenMap(),
	fCreator(creator),
	fError(B_OK)
{
}


BArchiveManager::~BArchiveManager()
{
	fTopLevelArchive->AddBool(kManagedField, true);
}


status_t
BArchiveManager::GetTokenForArchivable(BArchivable* archivable, int32& _token)
{
	if (!archivable) {
		_token = NULL_TOKEN;
		return B_OK;
	}

	TokenMap::iterator it = fTokenMap.find(archivable);

	if (it == fTokenMap.end())
		return B_ENTRY_NOT_FOUND;

	_token = it->second.token;
	return B_OK;
}


status_t
BArchiveManager::ArchiveObject(BArchivable* archivable,
	bool deep, int32& _token)
{
	if (!archivable) {
		_token = NULL_TOKEN;
		return B_OK;
	}

	ArchiveInfo& info = fTokenMap[archivable];

	status_t err = B_OK;

	if (!info.archive) {
		info.archive = new BMessage();
		info.token = fTokenMap.size() - 1;

		MarkArchive(info.archive);
		err = archivable->Archive(info.archive, deep);
	}

	if (err != B_OK) {
		fTokenMap.erase(archivable);
			// info.archive gets deleted here
		_token = NULL_TOKEN;
	} else
		_token = info.token;

	return err;
}


bool
BArchiveManager::IsArchived(BArchivable* archivable)
{
	if (!archivable)
		return true;

	return fTokenMap.find(archivable) != fTokenMap.end();
}


status_t
BArchiveManager::ArchiverLeaving(const BArchiver* archiver, status_t err)
{
	if (fError == B_OK)
		fError = err;

	if (archiver == fCreator && fError == B_OK) {
		// first, we must sort the objects into the order they were archived in
		typedef std::pair<BMessage*, const BArchivable*> ArchivePair;
		ArchivePair pairs[fTokenMap.size()];

		for(TokenMap::iterator it = fTokenMap.begin(), end = fTokenMap.end();
				it != end; it++) {
			ArchiveInfo& info = it->second;
			pairs[info.token].first = info.archive;
			pairs[info.token].second = it->first;

			// make sure fTopLevelArchive isn't deleted
			if (info.archive == fTopLevelArchive)
				info.archive = NULL;
		}

		int32 count = fTokenMap.size();
		for (int32 i = 0; i < count; i++) {
			const ArchivePair& pair = pairs[i];
			fError = pair.second->AllArchived(pair.first);

			if (fError == B_OK && i > 0) {
				fError = fTopLevelArchive->AddMessage(kArchivableField,
					pair.first);
			}

			if (fError != B_OK) {
				syslog(LOG_ERR, "AllArchived failed for object of type %s.",
					typeid(*pairs[i].second).name());
				break;
			}
		}
	}

	status_t result = fError;
	if (archiver == fCreator) 
		delete this;

	return result;
}


void
BArchiveManager::RegisterArchivable(const BArchivable* archivable)
{
	if (fTokenMap.size() == 0) {
		ArchiveInfo& info = fTokenMap[archivable];
		info.archive = fTopLevelArchive;
		info.token = 0;
	}
}


// #pragma mark -


struct BUnarchiveManager::ArchiveInfo {
	ArchiveInfo()
		:
		archivable(NULL),
		archive(),
		adopted(false)
	{
	}

	bool
	operator<(const ArchiveInfo& other)
	{
		return archivable < other.archivable;
	}

	BArchivable*	archivable;
	BMessage		archive;
	bool			adopted;
};


// #pragma mark -


BUnarchiveManager::BUnarchiveManager(BMessage* archive)
	:
	BManagerBase(archive, BManagerBase::UNARCHIVE_MANAGER),
	fObjects(NULL),
	fObjectCount(0),
	fTokenInProgress(0),
	fRefCount(0),
	fError(B_OK)
{
	archive->GetInfo(kArchivableField, NULL, &fObjectCount);
	fObjectCount++;
		// root object needs a spot too
	fObjects = new ArchiveInfo[fObjectCount];

	// fObjects[0] is a placeholder for the object that started
	// this unarchiving session.
	for (int32 i = 0; i < fObjectCount - 1; i++) {
		BMessage* into = &fObjects[i + 1].archive;
		status_t err = archive->FindMessage(kArchivableField, i, into);
		MarkArchive(into);

		if (err != B_OK)
			syslog(LOG_ERR, "Failed to find managed archivable");
	}
}


BUnarchiveManager::~BUnarchiveManager()
{
	delete[] fObjects;
}


status_t
BUnarchiveManager::GetArchivableForToken(int32 token,
	BUnarchiver::ownership_policy owning, BArchivable*& _archivable)
{
	if (token >= fObjectCount)
		return B_BAD_VALUE;

	if (token < 0) {
		_archivable = NULL;
		return B_OK;
	}

	status_t err = B_OK;
	ArchiveInfo& info = fObjects[token];
	if (!info.archivable) {
		if (fRefCount > 0) {
			fTokenInProgress = token;
			if(!instantiate_object(&info.archive))
				err = B_ERROR;
		} else {
			syslog(LOG_ERR, "Object requested from AllUnarchived()"
				" was not previously instantiated");
			err = B_ERROR;
		}
	}

	if (!info.adopted && owning == BUnarchiver::B_ASSUME_OWNERSHIP)
		info.adopted = true;
	else if (info.adopted && owning == BUnarchiver::B_ASSUME_OWNERSHIP)
		debugger("Cannot assume ownership of an object that is already owned");

	_archivable = info.archivable;
	return err;
}


bool
BUnarchiveManager::IsInstantiated(int32 token)
{
	if (token < 0 || token >= fObjectCount)
		return false;
	return fObjects[token].archivable;
}


void
BUnarchiveManager::RegisterArchivable(BArchivable* archivable)
{
	if (!archivable)
		debugger("Cannot register NULL pointer");

	fObjects[fTokenInProgress].archivable = archivable;
	archivable->fArchivingToken = fTokenInProgress;
}


status_t
BUnarchiveManager::UnarchiverLeaving(const BUnarchiver* unarchiver,
	status_t err)
{
	if (--fRefCount >= 0 && fError == B_OK)
		fError = err;

	if (fRefCount != 0)
		return fError;

	if (fError == B_OK) {
		BArchivable* archivable = fObjects[0].archivable;
		if (archivable) {
			fError = archivable->AllUnarchived(fTopLevelArchive);
			archivable->fArchivingToken = NULL_TOKEN;
		}

		for (int32 i = 1; i < fObjectCount && fError == B_OK; i++) {
			archivable = fObjects[i].archivable;
			if (archivable) {
				fError = archivable->AllUnarchived(&fObjects[i].archive);
				archivable->fArchivingToken = NULL_TOKEN;
			}
		}
		if (fError != B_OK) {
			syslog(LOG_ERR, "Error in AllUnarchived"
				" method of object of type %s", typeid(*archivable).name());
		}
	}

	if (fError != B_OK) {
		syslog(LOG_ERR, "An error occured during unarchival, cleaning up.");
		for (int32 i = 1; i < fObjectCount; i++) {
			if (!fObjects[i].adopted)
				delete fObjects[i].archivable;
		}
	}

	status_t result = fError;
	delete this;
	return result;
}


void
BUnarchiveManager::RelinquishOwnership(BArchivable* archivable)
{
	int32 token = NULL_TOKEN;
	if (archivable)
		token = archivable->fArchivingToken;

	if (token < 0 || token >= fObjectCount
		|| fObjects[token].archivable != archivable)
		return;

	fObjects[token].adopted = false;
}


void
BUnarchiveManager::AssumeOwnership(BArchivable* archivable)
{
	int32 token = NULL_TOKEN;
	if (archivable)
		token = archivable->fArchivingToken;

	if (token < 0 || token >= fObjectCount
		|| fObjects[token].archivable != archivable)
		return;

	if (!fObjects[token].adopted)
		fObjects[token].adopted = true;
	else {
		debugger("Cannot assume ownership of an object that is already owned");
	}
}


void
BUnarchiveManager::Acquire()
{
	if (fRefCount >= 0)
		fRefCount++;
}
