// StatisticsManager.cpp

#include <Message.h>

#include "AutoLocker.h"
#include "Debug.h"
#include "HashMap.h"
#include "SecurityContext.h"
#include "StatisticsManager.h"

typedef HashMap<String, int32>	UserCountMap;

// ShareStatistics
class StatisticsManager::ShareStatistics {
public:
	ShareStatistics(const char* share)
		: fShare(share),
		  fUsers()
	{
	}

	~ShareStatistics()
	{
	}

	status_t Init()
	{
		return fUsers.InitCheck();
	}

	const char* GetShare() const
	{
		return fShare.GetString();
	}

	void AddUser(const char* user)
	{
		int32 count = 0;
		if (fUsers.ContainsKey(user))
			count = fUsers.Get(user);
		count++;
		fUsers.Put(user, count);
	}

	void RemoveUser(const char* user)
	{
		if (!fUsers.ContainsKey(user))
			return;

		int32 count = fUsers.Get(user);
		count--;
		if (count > 0)
			fUsers.Put(user, count);
		else
			fUsers.Remove(user);
	}

	status_t GetStatistics(BMessage* statistics)
	{
		// add "mounted by"
		for (UserCountMap::Iterator it = fUsers.GetIterator(); it.HasNext();) {
			String user(it.Next().key);
			status_t error = statistics->AddString("mounted by",
				user.GetString());
			if (error != B_OK)
				return error;
		}
		return B_OK;
	}

private:
	String			fShare;
	UserCountMap	fUsers;
};

// ShareStatisticsMap
struct StatisticsManager::ShareStatisticsMap
	: HashMap<String, StatisticsManager::ShareStatistics*> {
};


// constructor
StatisticsManager::StatisticsManager()
	: fLock("statistics manager"),
	  fShareStatistics(NULL)
{
}

// destructor
StatisticsManager::~StatisticsManager()
{
	// delete the share statistics
	for (ShareStatisticsMap::Iterator it = fShareStatistics->GetIterator();
		 it.HasNext();) {
		ShareStatistics* statistics = it.Next().value;
		delete statistics;
	}

	delete fShareStatistics;
}

// Init
status_t
StatisticsManager::Init()
{
	// check lock
	if (fLock.Sem() < 0)
		return fLock.Sem();

	// create share info map
	fShareStatistics = new(nothrow) ShareStatisticsMap;
	if (!fShareStatistics)
		return B_NO_MEMORY;
	status_t error = fShareStatistics->InitCheck();
	if (error != B_OK)
		return error;

	return B_OK;
}

// CreateDefault
status_t
StatisticsManager::CreateDefault()
{
	if (fManager)
		return B_OK;

	fManager = new(nothrow) StatisticsManager;
	if (!fManager)
		return B_NO_MEMORY;
	status_t error = fManager->Init();
	if (error != B_OK) {
		DeleteDefault();
		return error;
	}

	return B_OK;
}

// DeleteDefault
void
StatisticsManager::DeleteDefault()
{
	if (fManager) {
		delete fManager;
		fManager = NULL;
	}
}

// GetDefault
StatisticsManager*
StatisticsManager::GetDefault()
{
	return fManager;
}

// UserRemoved
void
StatisticsManager::UserRemoved(User* user)
{
	// the shares the user mounted should already have been unmounted
}

// ShareRemoved
void
StatisticsManager::ShareRemoved(Share* share)
{
	if (!share)
		return;

	AutoLocker<Locker> locker(fLock);

	ShareStatistics* statistics = fShareStatistics->Remove(share->GetName());
	delete statistics;
}

// ShareMounted
void
StatisticsManager::ShareMounted(Share* share, User* user)
{
	if (!share || !user)
		return;

	AutoLocker<Locker> locker(fLock);

	// get the statistics
	ShareStatistics* statistics = fShareStatistics->Get(share->GetName());
	if (!statistics) {
		// no statistics for this share yet: create
		statistics = new(nothrow) ShareStatistics(share->GetName());
		if (!statistics)
			return;

		// add to the map
		if (fShareStatistics->Put(share->GetName(), statistics) != B_OK) {
			delete statistics;
			return;
		}
	}

	// add the user
	statistics->AddUser(user->GetName());
}

// ShareUnmounted
void
StatisticsManager::ShareUnmounted(Share* share, User* user)
{
	if (!share || !user)
		return;

	AutoLocker<Locker> locker(fLock);

	// get the statistics
	ShareStatistics* statistics = fShareStatistics->Get(share->GetName());
	if (!statistics)
		return;

	// remove the user
	statistics->RemoveUser(user->GetName());
}

// GetUserStatistics
status_t
StatisticsManager::GetUserStatistics(User* user, BMessage* statistics)
{
	if (!user)
		return B_BAD_VALUE;

	return GetUserStatistics(user->GetName(), statistics);
}

// GetUserStatistics
status_t
StatisticsManager::GetUserStatistics(const char* user, BMessage* _statistics)
{
	if (!user || !_statistics)
		return B_BAD_VALUE;

	// nothing for now

	return B_OK;
}

// GetShareStatistics
status_t
StatisticsManager::GetShareStatistics(Share* share, BMessage* statistics)
{
	if (!share)
		return B_BAD_VALUE;

	return GetShareStatistics(share->GetName(), statistics);
}

// GetShareStatistics
status_t
StatisticsManager::GetShareStatistics(const char* share, BMessage* _statistics)
{
	if (!share || !_statistics)
		return B_BAD_VALUE;

	AutoLocker<Locker> locker(fLock);

	// get the statistics
	ShareStatistics* statistics = fShareStatistics->Get(share);
	if (!statistics)
		return B_OK;

	// get the users
	return statistics->GetStatistics(_statistics);
}


// fManager
StatisticsManager*	StatisticsManager::fManager = NULL;
