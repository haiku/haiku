// StatCacheServerImpl.h

#ifndef STAT_CACHE_SERVER_IMPL_H
#define STAT_CACHE_SERVER_IMPL_H

#include <hash_map>
#include <string>

#include <Entry.h>
#include <MessageQueue.h>
#include <Node.h>
#include <OS.h>

class Directory;
class Entry;
class Node;
class SymLink;

// NodeRefHash
struct NodeRefHash
{
	size_t operator()(const node_ref &nodeRef) const;
};
 
// EntryRefHash
struct EntryRefHash
{
	size_t operator()(const entry_ref &entryRef) const;
}; 

// Referencable
class Referencable {
public:
	Referencable()
		: fReferenceCount(1),
		  fReferenceBaseCount(0)
	{
	}

	virtual ~Referencable()
	{
	}

	void AddReference()
	{
		fReferenceCount++;
	}

	bool RemoveReference()
	{
		if (--fReferenceCount <= fReferenceBaseCount) {
			Unreferenced();
			return true;
		}
		return false;
	}

	int32 CountReferences() const
	{
		return fReferenceCount;
	}

protected:
	virtual void Unreferenced() {};

protected:
	int32	fReferenceCount;
	int32	fReferenceBaseCount;
	bool	fDeleteWhenUnreferenced;
};

// Entry
class Entry : public Referencable {
public:
	Entry();
	~Entry();

	status_t SetTo(Directory *parent, const char *name);

	Directory *GetParent() const;

	const char *GetName() const;

	void SetNode(Node *node);
	Node *GetNode() const;

	void SetPrevious(Entry *entry);
	Entry *GetPrevious() const;
	void SetNext(Entry *entry);
	Entry *GetNext() const;

	entry_ref GetEntryRef() const;
	status_t GetPath(string& path);

protected:
	virtual void Unreferenced();

private:
	Directory	*fParent;
	string		fName;
	Node		*fNode;
	Entry		*fPrevious;
	Entry		*fNext;
};

// Node
class Node : public Referencable {
public:
	Node(const struct stat &st);
	virtual ~Node();

	virtual status_t SetTo(Entry *entry);

	status_t GetPath(string& path);

	const struct stat &GetStat() const;
	status_t GetStat(struct stat *st);
	status_t UpdateStat();
	void MarkStatInvalid();

	void SetEntry(Entry *entry);
	Entry *GetEntry() const;

	node_ref GetNodeRef() const;

protected:
	virtual void Unreferenced();

protected:
	Entry		*fEntry;
	struct stat	fStat;
	bool		fStatValid;
};

// Directory
class Directory : public Node {
public:
	Directory(const struct stat &st);
	~Directory();

	virtual status_t SetTo(Entry *entry);

	status_t FindEntry(const char *name, Entry **entry);
	Entry *GetFirstEntry() const;
	Entry *GetNextEntry(Entry *entry) const;

	status_t ReadAllEntries();

	bool IsComplete() const;

	void AddEntry(Entry *entry);
	void RemoveEntry(Entry *entry);

private:
	Entry	*fFirstEntry;
	Entry	*fLastEntry;
	bool	fIsComplete;
};

// SymLink
class SymLink : public Node {
public:
	SymLink(const struct stat &st);
	~SymLink();

	virtual status_t SetTo(Entry *entry);

	const char *GetTarget() const;

private:
	string	fTarget;
};

// NodeMonitor
class NodeMonitor : public BLooper {
public:
	NodeMonitor();
	virtual ~NodeMonitor();

	status_t Init();

	virtual void MessageReceived(BMessage *message);

	status_t StartWatching(Node *node);
	status_t StopWatching(Node *node);

	status_t GetNextMonitoringMessage(BMessage **message);

private:
	int32			fCurrentNodeMonitorLimit;
	BMessageQueue	fMessageQueue;
	sem_id			fMessageCountSem;
};

// PathResolver
class PathResolver {
public:
	PathResolver();

	status_t FindEntry(const char *path, bool traverse, Entry **_entry);
	status_t FindEntry(Entry *entry, const char *path, bool traverse,
		Entry **_entry);
	status_t FindNode(const char *path, bool traverse, Node **node);

	status_t ResolveSymlink(Node *node, Node **_node);
	status_t ResolveSymlink(Node *node, Entry **entry);
	status_t ResolveSymlink(Entry *entry, Entry **_entry);

private:
	int32	fSymLinkCounter;
};

// NodeManager
class NodeManager : public BLocker {
public:
	NodeManager();
	~NodeManager();

	static NodeManager *GetDefault();

	status_t Init();

	Directory *GetRootDirectory() const;

	Node *GetNode(const node_ref &nodeRef);
	Entry *GetEntry(const entry_ref &entryRef);

	status_t CreateEntry(const entry_ref &entryRef, Entry **_entry);
	status_t CreateDirectory(const node_ref &nodeRef, Directory **_dir);

	void RemoveEntry(Entry *entry);
	void MoveEntry(Entry *entry, const entry_ref &newRef);

	void EntryUnreferenced(Entry *entry);
	void NodeUnreferenced(Node *node);

	status_t StartWatching(Node *node);
	status_t StopWatching(Node *node);

private:
	static int32 _NodeMonitoringProcessorEntry(void *data);
	int32 _NodeMonitoringProcessor();

	status_t _CreateNode(Entry *entry, Node **_node);

	void _EntryCreated(BMessage *message);
	void _EntryRemoved(BMessage *message);
	void _EntryMoved(BMessage *message);
	void _StatChanged(BMessage *message);

private:
	typedef hash_map<entry_ref, Entry*, EntryRefHash> EntryMap;
	typedef hash_map<node_ref, Node*, NodeRefHash> NodeMap;

	EntryMap	fEntries;
	NodeMap		fNodes;
	Directory	*fRootDirectory;
	NodeMonitor	*fNodeMonitor;
	thread_id	fNodeMonitoringProcessor;

	static NodeManager	sManager;
};

#endif	// STAT_CACHE_SERVER_IMPL_H
