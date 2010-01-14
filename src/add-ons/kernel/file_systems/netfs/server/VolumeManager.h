// VolumeManager.h

#ifndef NET_FS_VOLUME_MANAGER_H
#define NET_FS_VOLUME_MANAGER_H

#include <AutoLocker.h>
#include <Entry.h>
#include <Locker.h>
#include <util/DoublyLinkedList.h>

#include "BlockingQueue.h"
#include "NodeMonitor.h"
#include "NodeMonitoringEvent.h"

class ClientVolume;
class Directory;
class Entry;
class Node;
class Path;
class QueryHandle;
class QueryDomain;
class Volume;

// VolumeManager
class VolumeManager : private NodeMonitorListener {
private:
								VolumeManager();
								~VolumeManager();

			status_t			Init();

public:
	static	status_t			CreateDefault();
	static	void				DeleteDefault();
	static	VolumeManager*		GetDefault();

			bool				Lock();
			void				Unlock();

			int64				GetRevision() const;

			Volume*				GetVolume(dev_t volumeID, bool add = false);
			Volume*				GetRootVolume() const;

			status_t			AddClientVolume(ClientVolume* clientVolume);
			void				RemoveClientVolume(ClientVolume* clientVolume);

			status_t			AddNode(Node* node);
			void				RemoveNode(Node* node);
			Node*				GetNode(dev_t volumeID, ino_t nodeID);
			status_t			LoadNode(const struct stat& st, Node** node);

			Directory*			GetDirectory(dev_t volumeID, ino_t nodeID);
			Directory*			GetRootDirectory() const;
			Directory*			GetParentDirectory(Directory* directory);
			status_t			LoadDirectory(dev_t volumeID, ino_t directoryID,
									Directory** directory);

			status_t			AddEntry(Entry* entry);
			void				RemoveEntry(Entry* entry);
			void				DeleteEntry(Entry* entry, bool keepNode);
			Entry*				GetEntry(dev_t volumeID, ino_t directoryID,
									const char* name);
			Entry*				GetEntry(const entry_ref& ref);
			status_t			LoadEntry(dev_t volumeID, ino_t directoryID,
									const char* name, bool loadDir,
									Entry** entry);

			status_t			OpenQuery(QueryDomain* queryDomain,
									const char* queryString, uint32 flags,
									port_id remotePort, int32 remoteToken,
									QueryHandle** handle);

			status_t			CompletePathToRoot(Directory* directory);

			status_t			GetPath(Entry* entry, Path* path);
			status_t			GetPath(Node* node, Path* path);

			bool				DirectoryContains(Directory* directory,
									Entry* entry);
			bool				DirectoryContains(Directory* directory,
									Directory* descendant, bool reflexive);
			bool				DirectoryContains(Directory* directory,
									Node* descendant, bool reflexive);

private:
	virtual	void				ProcessNodeMonitoringEvent(
									NodeMonitoringEvent* event);

			status_t			_AddVolume(dev_t volumeID,
									Volume** volume = NULL);

			void				_EntryCreated(EntryCreatedEvent* event);
			void				_EntryRemoved(EntryRemovedEvent* event,
									bool keepNode);
			void				_EntryMoved(EntryMovedEvent* event);
			void				_NodeStatChanged(StatChangedEvent* event);
			void				_NodeAttributeChanged(
									AttributeChangedEvent* event);
			void				_VolumeMounted(VolumeMountedEvent* event);
			void				_VolumeUnmounted(VolumeUnmountedEvent* event);

			void				_QueryEntryCreated(EntryCreatedEvent* event);
			void				_QueryEntryRemoved(EntryRemovedEvent* event);
			void				_QueryEntryMoved(EntryMovedEvent* event);

			bool				_IsRecentEvent(
									NodeMonitoringEvent* event) const;

			status_t			_GenerateEntryCreatedEvent(const entry_ref& ref,
									bigtime_t time,
									EntryCreatedEvent** event = NULL);
			status_t			_GenerateEntryRemovedEvent(Entry* entry,
									bigtime_t time,
									EntryRemovedEvent** event = NULL);

			void				_CheckVolumeRootMoved(EntryMovedEvent* event);

	static	int32				_NodeMonitoringProcessorEntry(void* data);
			int32				_NodeMonitoringProcessor();

private:
			class QueryHandler;
			struct VolumeMap;
			struct ClientVolumeMap;
			typedef BlockingQueue<NodeMonitoringEvent> NodeMonitoringEventQueue;
			typedef DoublyLinkedList<NodeMonitoringEvent>
				NodeMonitoringEventList;
			struct EntryCreatedEventMap;
			struct EntryRemovedEventMap;
			struct EntryMovedEventMap;
			struct NodeStatChangedEventMap;
			struct NodeAttributeChangedEventMap;

			BLocker				fLock;
			VolumeMap*			fVolumes;
			Volume*				fRootVolume;
			ClientVolumeMap*	fClientVolumes;
			NodeMonitor*		fNodeMonitor;
			thread_id			fNodeMonitoringProcessor;
			NodeMonitoringEventQueue fNodeMonitoringEvents;
			NodeMonitoringEventList fRecentNodeMonitoringEvents;
			EntryCreatedEventMap* fEntryCreatedEvents;
			EntryRemovedEventMap* fEntryRemovedEvents;
			EntryMovedEventMap*	fEntryMovedEvents;
			NodeStatChangedEventMap* fNodeStatChangedEvents;
			NodeAttributeChangedEventMap* fNodeAttributeChangedEvents;
			int64				fRevision;
			bool				fTerminating;

	static	VolumeManager*		sManager;
};

// VolumeManagerLocker
struct VolumeManagerLocker : AutoLocker<VolumeManager> {
	VolumeManagerLocker()
		: AutoLocker<VolumeManager>(VolumeManager::GetDefault())
	{
	}
};

#endif	// NET_FS_VOLUME_MANAGER_H
