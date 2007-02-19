// FDManager.h

#ifndef NET_FS_FD_MANAGER_H
#define NET_FS_FD_MANAGER_H

#include <SupportDefs.h>

#include "Locker.h"

struct DIR;
class BDirectory;
class BEntry;
class BFile;
class BNode;

class FDManager {
private:
								FDManager();
								~FDManager();

			status_t			Init();

public:
	static	status_t			CreateDefault();
	static	void				DeleteDefault();
	static	FDManager*			GetDefault();

	static	status_t			SetDirectory(BDirectory* directory,
									const node_ref* ref);
	static	status_t			SetEntry(BEntry* entry, const entry_ref* ref);
	static	status_t			SetEntry(BEntry* entry, const char* path);
	static	status_t			SetFile(BFile* file, const char* path,
									uint32 openMode);
	static	status_t			SetNode(BNode* node, const entry_ref* ref);

	static	status_t			Open(const char* path, uint32 openMode,
									mode_t mode, int& fd);
	static	status_t			OpenDir(const char* path, DIR*& dir);
	static	status_t			OpenAttrDir(const char* path, DIR*& dir);

private:
			status_t			_IncreaseLimit();

private:
			Locker				fLock;
			int32				fFDLimit;

	static	FDManager*			sManager;
};

#endif	// NET_FS_FD_MANAGER_H
