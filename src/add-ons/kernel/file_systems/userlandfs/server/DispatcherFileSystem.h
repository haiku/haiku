// FileSystem.h

#ifndef USERLAND_FS_DISPATCHER_FILE_SYSTEM_H
#define USERLAND_FS_DISPATCHER_FILE_SYSTEM_H

#include <Locker.h>

#include "FSInfo.h"
#include "LazyInitializable.h"
#include "Referencable.h"
#include "String.h"

namespace UserlandFS {
namespace Dispatcher {

class FileSystem : public LazyInitializable, public Referencable {
public:
								FileSystem(const char* name, status_t* error);
								FileSystem(team_id team, FSInfo* info,
									status_t* error);
								~FileSystem();

			const char*			GetName() const;
			const FSInfo*		GetInfo() const;
			team_id				GetTeam() const;

			void				CompleteInit(FSInfo* info);
			void				AbortInit();

private:
	virtual	status_t			FirstTimeInit();

			status_t			_WaitForInitToFinish();

private:
			String				fName;
			FSInfo*				fInfo;
			team_id				fTeam;
			sem_id				fFinishInitSemaphore;
	mutable	BLocker				fTeamLock;
};

}	// namespace Dispatcher
}	// namespace UserlandFS

using UserlandFS::Dispatcher::FileSystem;

#endif	// USERLAND_FS_DISPATCHER_FILE_SYSTEM_H
