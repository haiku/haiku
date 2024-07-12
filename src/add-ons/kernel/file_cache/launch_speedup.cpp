/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/** This module memorizes all opened files for a certain session. A session
 *	can be the start of an application or the boot process.
 *	When a session is started, it will prefetch all files from an earlier
 *	session in order to speed up the launching or booting process.
 *
 *	Note: this module is using private kernel API and is definitely not
 *		meant to be an example on how to write modules.
 */


#include "launch_speedup.h"

#include <KernelExport.h>
#include <Node.h>

#include <util/kernel_cpp.h>
#include <util/AutoLock.h>
#include <thread.h>
#include <team.h>
#include <file_cache.h>
#include <generic_syscall.h>
#include <syscalls.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

extern dev_t gBootDevice;


// ToDo: combine the last 3-5 sessions to their intersection
// ToDo: maybe ignore sessions if the node count is < 3 (without system libs)

#define TRACE_CACHE_MODULE
#ifdef TRACE_CACHE_MODULE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define VNODE_HASH(mountid, vnodeid) (((uint32)((vnodeid) >> 32) \
	+ (uint32)(vnodeid)) ^ (uint32)(mountid))

struct data_part {
	off_t		offset;
	off_t		size;
};

struct node {
	struct node	*next;
	node_ref	ref;
	int32		ref_count;
	bigtime_t	timestamp;
	data_part	parts[5];
	size_t		part_count;
};

struct NodeHash {
	typedef node_ref	KeyType;
	typedef	node		ValueType;

	size_t HashKey(KeyType key) const
	{
		return VNODE_HASH(key.device, key.node);
	}

	size_t Hash(ValueType* value) const
	{
		return HashKey(value->ref);
	}

	bool Compare(KeyType key, ValueType* node) const
	{
		return (node->ref.device == key.device && node->ref.node == key.node);
	}

	ValueType*& GetLink(ValueType* value) const
	{
		return value->next;
	}
};

typedef BOpenHashTable<NodeHash> NodeTable;

class Session {
	public:
		Session(team_id team, const char *name, dev_t device,
			ino_t node, int32 seconds);
		Session(const char *name);
		~Session();

		status_t InitCheck();
		team_id Team() const { return fTeam; }
		const char *Name() const { return fName; }
		const node_ref &NodeRef() const { return fNodeRef; }
		bool IsActive() const { return fActiveUntil >= system_time(); }
		bool IsClosing() const { return fClosing; }
		bool IsMainSession() const;
		bool IsWorthSaving() const;

		void AddNode(dev_t device, ino_t node);
		void RemoveNode(dev_t device, ino_t node);

		void Lock() { mutex_lock(&fLock); }
		void Unlock() { mutex_unlock(&fLock); }

		status_t StartWatchingTeam();
		void StopWatchingTeam();

		status_t LoadFromDirectory(int fd);
		status_t Save();
		void Prefetch();

		Session *&Next() { return fNext; }

	private:
		struct node *_FindNode(dev_t device, ino_t node);

		Session		*fNext;
		char		fName[B_OS_NAME_LENGTH];
		mutex		fLock;
		NodeTable	*fNodeHash;
		struct node	*fNodes;
		int32		fNodeCount;
		team_id		fTeam;
		node_ref	fNodeRef;
		bigtime_t	fActiveUntil;
		bigtime_t	fTimestamp;
		bool		fClosing;
		bool		fIsWatchingTeam;
};

class SessionGetter {
	public:
		SessionGetter(team_id team, Session **_session);
		~SessionGetter();

		status_t New(const char *name, dev_t device, ino_t node,
					Session **_session);
		void Stop();

	private:
		Session	*fSession;
};

static Session *sMainSession;
static SessionTable *sTeamHash;
static PrefetchTable *sPrefetchHash;
static Session *sMainPrefetchSessions;
	// singly-linked list
static recursive_lock sLock;


node_ref::node_ref()
{
	// part of libbe.so
}


struct PrefetchHash {
	typedef node_ref	KeyType;
	typedef	Session		ValueType;

	size_t HashKey(KeyType key) const
	{
		return VNODE_HASH(key.device, key.node);
	}

	size_t Hash(ValueType* value) const
	{
		return HashKey(value->NodeRef());
	}

	bool Compare(KeyType key, ValueType* session) const
	{
		return (session->NodeRef().device == key.device
			&& session->NodeRef().node == key.node);
	}

	ValueType*& GetLink(ValueType* value) const
	{
		return value->Next();
	}
};

typedef BOpenHashTable<PrefetchHash> PrefetchTable;


struct SessionHash {
	typedef team_id		KeyType;
	typedef	Session		ValueType;

	size_t HashKey(KeyType key) const
	{
		return key;
	}

	size_t Hash(ValueType* value) const
	{
		return HashKey(value->Team());
	}

	bool Compare(KeyType key, ValueType* session) const
	{
		return session->Team == key;
	}

	ValueType*& GetLink(ValueType* value) const
	{
		return value->Next();
	}
};

typedef BOpenHashTable<SessionHash> SessionTable;


static void
stop_session(Session *session)
{
	if (session == NULL)
		return;

	TRACE(("stop_session(%s)\n", session->Name()));

	if (session->IsWorthSaving())
		session->Save();

	{
		RecursiveLocker locker(&sLock);

		if (session->Team() >= B_OK)
			sTeamHash->Remove(session);

		if (session == sMainSession)
			sMainSession = NULL;
	}

	delete session;
}


static Session *
start_session(team_id team, dev_t device, ino_t node, const char *name,
	int32 seconds = 30)
{
	RecursiveLocker locker(&sLock);

	Session *session = new Session(team, name, device, node, seconds);
	if (session == NULL)
		return NULL;

	if (session->InitCheck() != B_OK || session->StartWatchingTeam() != B_OK) {
		delete session;
		return NULL;
	}

	// let's see if there is a prefetch session for this session

	Session *prefetchSession;
	if (session->IsMainSession()) {
		// search for session by name
		for (prefetchSession = sMainPrefetchSessions;
				prefetchSession != NULL;
				prefetchSession = prefetchSession->Next()) {
			if (!strcmp(prefetchSession->Name(), name)) {
				// found session!
				break;
			}
		}
	} else {
		// ToDo: search for session by device/node ID
		prefetchSession = NULL;
	}
	if (prefetchSession != NULL) {
		TRACE(("found prefetch session %s\n", prefetchSession->Name()));
		prefetchSession->Prefetch();
	}

	if (team >= B_OK)
		sTeamHash->Insert(session);

	session->Lock();
	return session;
}


static void
team_gone(team_id team, void *_session)
{
	Session *session = (Session *)_session;

	session->Lock();
	stop_session(session);
}


static bool
parse_node_ref(const char *string, node_ref &ref, const char **_end = NULL)
{
	// parse node ref
	char *end;
	ref.device = strtol(string, &end, 0);
	if (end == NULL || ref.device == 0)
		return false;

	ref.node = strtoull(end + 1, &end, 0);

	if (_end)
		*_end = end;
	return true;
}


static struct node *
new_node(dev_t device, ino_t id)
{
	struct node *node = new ::node;
	if (node == NULL)
		return NULL;

	node->ref.device = device;
	node->ref.node = id;
	node->timestamp = system_time();

	return node;
}


static void
load_prefetch_data()
{
	DIR *dir = opendir("/etc/launch_cache");
	if (dir == NULL)
		return;

	struct dirent *dirent;
	while ((dirent = readdir(dir)) != NULL) {
		if (dirent->d_name[0] == '.')
			continue;

		Session *session = new Session(dirent->d_name);

		if (session->LoadFromDirectory(dirfd(dir)) != B_OK) {
			delete session;
			continue;
		}

		if (session->IsMainSession()) {
			session->Next() = sMainPrefetchSessions;
			sMainPrefetchSessions = session;
		} else {
			sPrefetchHash->Insert(session);
		}
	}

	closedir(dir);
}


//	#pragma mark -


Session::Session(team_id team, const char *name, dev_t device,
	ino_t node, int32 seconds)
	:
	fNodes(NULL),
	fNodeCount(0),
	fTeam(team),
	fClosing(false),
	fIsWatchingTeam(false)
{
	if (name != NULL) {
		size_t length = strlen(name) + 1;
		if (length > B_OS_NAME_LENGTH)
			name += length - B_OS_NAME_LENGTH;

		strlcpy(fName, name, B_OS_NAME_LENGTH);
	} else
		fName[0] = '\0';

	mutex_init(&fLock, "launch speedup session");
	fNodeHash = new(std::nothrow) NodeTable();
	if (fNodeHash && fNodeHash->Init(64) != B_OK) {
		delete fNodeHash;
		fNodeHash = NULL;
	}
	fActiveUntil = system_time() + seconds * 1000000LL;
	fTimestamp = system_time();

	fNodeRef.device = device;
	fNodeRef.node = node;

	TRACE(("start session %ld:%lld \"%s\", system_time: %lld, active until: %lld\n",
		device, node, Name(), system_time(), fActiveUntil));
}


Session::Session(const char *name)
	:
	fNodeHash(NULL),
	fNodes(NULL),
	fClosing(false),
	fIsWatchingTeam(false)
{
	fTeam = -1;
	fNodeRef.device = -1;
	fNodeRef.node = -1;

	if (isdigit(name[0]))
		parse_node_ref(name, fNodeRef);

	strlcpy(fName, name, B_OS_NAME_LENGTH);
}


Session::~Session()
{
	mutex_destroy(&fLock);

	// free all nodes
	struct node *node, *next = NULL;

	if (fNodeHash) {
		// ... from the hash
		node = fNodeHash->Clear(true);
	} else {
		// ... from the list
		node = fNodes;
	}

	for (; node != NULL; node = next) {
		next = node->next;
		free(node);
	}

	delete fNodeHash;
	StopWatchingTeam();
}


status_t
Session::InitCheck()
{
	if (fNodeHash == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


node *
Session::_FindNode(dev_t device, ino_t node)
{
	node_ref key;
	key.device = device;
	key.node = node;

	return fNodeHash->Lookup(key);
}


void
Session::AddNode(dev_t device, ino_t id)
{
	struct node *node = _FindNode(device, id);
	if (node != NULL) {
		node->ref_count++;
		return;
	}

	node = new_node(device, id);
	if (node == NULL)
		return;

	fNodeHash->Insert(node);
	fNodeCount++;
}


void
Session::RemoveNode(dev_t device, ino_t id)
{
	struct node *node = _FindNode(device, id);
	if (node != NULL && --node->ref_count <= 0) {
		fNodeHash->Remove(node);
		fNodeCount--;
	}
}


status_t
Session::StartWatchingTeam()
{
	if (Team() < B_OK)
		return B_OK;

	status_t status = start_watching_team(Team(), team_gone, this);
	if (status == B_OK)
		fIsWatchingTeam = true;

	return status;
}


void
Session::StopWatchingTeam()
{
	if (fIsWatchingTeam)
		stop_watching_team(Team(), team_gone, this);
}


void
Session::Prefetch()
{
	if (fNodes == NULL || fNodeHash != NULL)
		return;

	for (struct node *node = fNodes; node != NULL; node = node->next) {
		cache_prefetch(node->ref.device, node->ref.node, 0, ~0UL);
	}
}


status_t
Session::LoadFromDirectory(int directoryFD)
{
	TRACE(("load session %s\n", Name()));

	int fd = _kern_open(directoryFD, Name(), O_RDONLY, 0);
	if (fd < B_OK)
		return fd;

	struct stat stat;
	if (fstat(fd, &stat) != 0) {
		close(fd);
		return errno;
	}

	if (stat.st_size > 32768) {
		// for safety reasons
		// ToDo: make a bit larger later
		close(fd);
		return B_BAD_DATA;
	}

	char *buffer = (char *)malloc(stat.st_size);
	if (buffer == NULL) {
		close(fd);
		return B_NO_MEMORY;
	}

	if (read(fd, buffer, stat.st_size) < stat.st_size) {
		free(buffer);
		close(fd);
		return B_ERROR;
	}

	const char *line = buffer;
	node_ref nodeRef;
	while (parse_node_ref(line, nodeRef, &line)) {
		struct node *node = new_node(nodeRef.device, nodeRef.node);
		if (node != NULL) {
			// note: this reverses the order of the nodes in the file
			node->next = fNodes;
			fNodes = node;
		}
		line++;
	}

	free(buffer);
	close(fd);
	return B_OK;
}


status_t
Session::Save()
{
	fClosing = true;

	char name[B_OS_NAME_LENGTH + 25];
	if (!IsMainSession()) {
		snprintf(name, sizeof(name), "/etc/launch_cache/%ld:%lld %s",
			fNodeRef.device, fNodeRef.node, Name());
	} else
		snprintf(name, sizeof(name), "/etc/launch_cache/%s", Name());

	int fd = open(name, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd < B_OK)
		return errno;

	status_t status = B_OK;
	off_t fileSize = 0;

	// ToDo: order nodes by timestamp... (should improve launch speed)
	// ToDo: test which parts of a file have been read (and save that as well)

	// enlarge file, so that it can be written faster
	ftruncate(fd, 512 * 1024);

	NodeTable::Iterator iterator(fNodeHash);
	while (iterator.HasNext()) {
		struct node *node = iterator.Next();
		snprintf(name, sizeof(name), "%ld:%lld\n", node->ref.device, node->ref.node);

		ssize_t bytesWritten = write(fd, name, strlen(name));
		if (bytesWritten < B_OK) {
			status = bytesWritten;
			break;
		}

		fileSize += bytesWritten;
	}

	ftruncate(fd, fileSize);
	close(fd);

	return status;
}


bool
Session::IsWorthSaving() const
{
	// ToDo: sort out entries with only very few nodes, and those that load
	//	instantly, anyway
	if (fNodeCount < 5 || system_time() - fTimestamp < 400000) {
		// sort anything out that opens less than 5 files, or needs less
		// than 0.4 seconds to load an run
		return false;
	}
	return true;
}


bool
Session::IsMainSession() const
{
	return fNodeRef.node == -1;
}


//	#pragma mark -


SessionGetter::SessionGetter(team_id team, Session **_session)
{
	RecursiveLocker locker(&sLock);

	if (sMainSession != NULL)
		fSession = sMainSession;
	else
		fSession = sTeamHash->Lookup(team);

	if (fSession != NULL) {
		if (!fSession->IsClosing())
			fSession->Lock();
		else
			fSession = NULL;
	}

	*_session = fSession;
}


SessionGetter::~SessionGetter()
{
	if (fSession != NULL)
		fSession->Unlock();
}


status_t
SessionGetter::New(const char *name, dev_t device, ino_t node,
	Session **_session)
{
	Thread *thread = thread_get_current_thread();
	fSession = start_session(thread->team->id, device, node, name);

	if (fSession != NULL) {
		*_session = fSession;
		return B_OK;
	}

	return B_ERROR;
}


void
SessionGetter::Stop()
{
	if (fSession == sMainSession)
		sMainSession = NULL;

	stop_session(fSession);
	fSession = NULL;
}

//	#pragma mark -


static void
node_opened(struct vnode *vnode, dev_t device, ino_t parent,
	ino_t node, const char *name, off_t size)
{
	if (device < gBootDevice) {
		// we ignore any access to rootfs, pipefs, and devfs
		// ToDo: if we can ever move the boot device on the fly, this will break
		return;
	}

	Session *session;
	SessionGetter getter(team_get_current_team_id(), &session);

	if (session == NULL) {
		char buffer[B_FILE_NAME_LENGTH];
		if (name == NULL
			&& vfs_get_vnode_name(vnode, buffer, sizeof(buffer)) == B_OK)
			name = buffer;

		// create new session for this team
		getter.New(name, device, node, &session);
	}

	if (session == NULL || !session->IsActive()) {
		if (sMainSession != NULL) {
			// ToDo: this opens a race condition with the "stop session" syscall
			getter.Stop();
		}
		return;
	}

	session->AddNode(device, node);
}


static void
node_closed(struct vnode *vnode, dev_t device, ino_t node,
	int32 accessType)
{
	Session *session;
	SessionGetter getter(team_get_current_team_id(), &session);

	if (session == NULL)
		return;

	if (accessType == FILE_CACHE_NO_IO)
		session->RemoveNode(device, node);
}


static status_t
launch_speedup_control(const char *subsystem, uint32 function,
	void *buffer, size_t bufferSize)
{
	switch (function) {
		case LAUNCH_SPEEDUP_START_SESSION:
		{
			char name[B_OS_NAME_LENGTH];
			if (!IS_USER_ADDRESS(buffer)
				|| user_strlcpy(name, (const char *)buffer, B_OS_NAME_LENGTH) < B_OK)
				return B_BAD_ADDRESS;

			if (isdigit(name[0]) || name[0] == '.')
				return B_BAD_VALUE;

			sMainSession = start_session(-1, -1, -1, name, 60);
			sMainSession->Unlock();
			return B_OK;
		}

		case LAUNCH_SPEEDUP_STOP_SESSION:
		{
			char name[B_OS_NAME_LENGTH];
			if (!IS_USER_ADDRESS(buffer)
				|| user_strlcpy(name, (const char *)buffer, B_OS_NAME_LENGTH) < B_OK)
				return B_BAD_ADDRESS;

			// ToDo: this check is not thread-safe
			if (sMainSession == NULL || strcmp(sMainSession->Name(), name))
				return B_BAD_VALUE;

			if (!strcmp(name, "system boot"))
				dprintf("STOP BOOT %lld\n", system_time());

			sMainSession->Lock();
			stop_session(sMainSession);
			sMainSession = NULL;
			return B_OK;
		}
	}

	return B_BAD_VALUE;
}


static void
uninit()
{
	unregister_generic_syscall(LAUNCH_SPEEDUP_SYSCALLS, 1);

	recursive_lock_lock(&sLock);

	// free all sessions from the hashes

	Session *session = sTeamHash->Clear(true);
	while (session != NULL) {
		Session *next = session->next;
		delete session;
		session = next;
	}
	session = sPrefetchHash->Clear(true);
	while (session != NULL) {
		Session *next = session->next;
		delete session;
		session = next;
	}

	// free all sessions from the main prefetch list

	for (session = sMainPrefetchSessions; session != NULL; ) {
		sMainPrefetchSessions = session->Next();
		delete session;
		session = sMainPrefetchSessions;
	}

	delete sTeamHash;
	delete sPrefetchHash;
	recursive_lock_destroy(&sLock);
}


static status_t
init()
{
	sTeamHash = new(std::nothrow) SessionTable();
	if (sTeamHash == NULL || sTeamHash->Init(64) != B_OK)
		return B_NO_MEMORY;

	status_t status;

	sPrefetchHash = new(std::nothrow) PrefetchTable();
	if (sPrefetchHash == NULL || sPrefetchHash->Init(64) != B_OK) {
		status = B_NO_MEMORY;
		goto err1;
	}

	recursive_lock_init(&sLock, "launch speedup");

	// register kernel syscalls
	if (register_generic_syscall(LAUNCH_SPEEDUP_SYSCALLS,
			launch_speedup_control, 1, 0) != B_OK) {
		status = B_ERROR;
		goto err3;
	}

	// read in prefetch knowledge base

	mkdir("/etc/launch_cache", 0755);
	load_prefetch_data();

	// start boot session

	sMainSession = start_session(-1, -1, -1, "system boot");
	sMainSession->Unlock();
	dprintf("START BOOT %lld\n", system_time());
	return B_OK;

err3:
	recursive_lock_destroy(&sLock);
	delete sPrefetchHash;
err1:
	delete sTeamHash;
	return status;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return init();

		case B_MODULE_UNINIT:
			uninit();
			return B_OK;

		default:
			return B_ERROR;
	}
}


static struct cache_module_info sLaunchSpeedupModule = {
	{
		CACHE_MODULES_NAME "/launch_speedup/v1",
		0,
		std_ops,
	},
	node_opened,
	node_closed,
	NULL,
};


module_info *modules[] = {
	(module_info *)&sLaunchSpeedupModule,
	NULL
};
