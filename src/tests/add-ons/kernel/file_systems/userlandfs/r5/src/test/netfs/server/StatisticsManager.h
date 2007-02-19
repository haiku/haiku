// StatisticsManager.h

#ifndef NET_FS_STATISTICS_MANAGER_H
#define NET_FS_STATISTICS_MANAGER_H

#include "Locker.h"

class BMessage;

class Share;
class User;

class StatisticsManager {
private:
								StatisticsManager();
								~StatisticsManager();

			status_t			Init();

public:
	static	status_t			CreateDefault();
	static	void				DeleteDefault();
	static	StatisticsManager*	GetDefault();

			void				UserRemoved(User* user);
			void				ShareRemoved(Share* share);

			void				ShareMounted(Share* share, User* user);
			void				ShareUnmounted(Share* share, User* user);

			status_t			GetUserStatistics(User* user,
									BMessage* statistics);
			status_t			GetUserStatistics(const char* user,
									BMessage* statistics);

			status_t			GetShareStatistics(Share* share,
									BMessage* statistics);
			status_t			GetShareStatistics(const char* share,
									BMessage* statistics);

private:
			class ShareStatistics;
			struct ShareStatisticsMap;

			Locker				fLock;
			ShareStatisticsMap*	fShareStatistics;

	static	StatisticsManager*	fManager;
};

#endif	// NET_FS_STATISTICS_MANAGER_H
