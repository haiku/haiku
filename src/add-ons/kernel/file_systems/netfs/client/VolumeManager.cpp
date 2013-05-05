// VolumeManager.cpp

#include <HashMap.h>
#include <HashSet.h>
#include <util/DoublyLinkedList.h>

#include "DebugSupport.h"
#include "QueryManager.h"
#include "RootVolume.h"
#include "VolumeEvent.h"
#include "VolumeManager.h"

// VolumeSet
struct VolumeManager::VolumeSet : HashSet<HashKey32<Volume*> > {
};

// NodeIDVolumeMap
struct VolumeManager::NodeIDVolumeMap : HashMap<HashKey64<vnode_id>, Volume*> {
};

// VolumeEventQueue
class VolumeManager::VolumeEventQueue {
public:
	VolumeEventQueue()
		: fLock("volume event queue"),
		  fCounterSem(-1)
	{
		fCounterSem = create_sem(0, "volume event count");
		#if !USER
			if (fCounterSem >= 0)
				set_sem_owner(fCounterSem, B_SYSTEM_TEAM);
		#endif
	}

	~VolumeEventQueue()
	{
		Close();
	}

	status_t InitCheck() const
	{
		if (fCounterSem < 0)
			return fCounterSem;
		return B_OK;
	}

	void Close()
	{
		AutoLocker<Locker> _(fLock);
		if (fCounterSem >= 0) {
			delete_sem(fCounterSem);
			fCounterSem = -1;
		}

		while (VolumeEvent* event = fEvents.First()) {
			fEvents.Remove(event);
			event->ReleaseReference();
		}
	}

	void Push(VolumeEvent* event)
	{
		if (!event)
			return;

		AutoLocker<Locker> _(fLock);
		if (fCounterSem < 0)
			return;
		fEvents.Insert(event);
		event->AcquireReference();
		release_sem(fCounterSem);
	}

	VolumeEvent* Pop()
	{
		status_t error;
		do {
			error = acquire_sem(fCounterSem);
		} while (error == B_INTERRUPTED);
		if (error != B_OK)
			return NULL;

		AutoLocker<Locker> _(fLock);
		if (VolumeEvent* event = fEvents.First()) {
			fEvents.Remove(event);
			return event;
		}

		return NULL;
	}

private:
	Locker				fLock;
	sem_id				fCounterSem;
	DoublyLinkedList<VolumeEvent> fEvents;
};


// constructor
VolumeManager::VolumeManager(nspace_id id, uint32 flags)
	: Locker("volume manager"),
	  fID(id),
	  fMountFlags(flags),
	  fMountUID(0),
	  fMountGID(0),
	  fRootID(-1),
	  fNextNodeID(2),
	  fQueryManager(NULL),
	  fVolumes(NULL),
	  fNodeIDs2Volumes(NULL),
	  fVolumeEvents(NULL),
	  fEventDeliverer(-1)
{
}

// destructor
VolumeManager::~VolumeManager()
{
	if (fVolumeEvents)
		fVolumeEvents->Close();
	if (fEventDeliverer >= 0) {
		int32 result;
		wait_for_thread(fEventDeliverer, &result);
	}
	delete fVolumeEvents;
	delete fVolumes;
	delete fNodeIDs2Volumes;
	delete fQueryManager;
}

// MountRootVolume
status_t
VolumeManager::MountRootVolume(const char* device,
	const char* parameters, int32 len, Volume** volume)
{
	// Store the uid/gid of the mounting user -- when running in userland
	// this is always the owner of the UserlandFS server, but we can't help
	// that.
	fMountUID = geteuid();
	fMountGID = getegid();

	// create the query manager
	fQueryManager = new(std::nothrow) QueryManager(this);
	if (!fQueryManager)
		return B_NO_MEMORY;
	status_t error = fQueryManager->Init();
	if (error != B_OK)
		return error;

	// create volumes set
	fVolumes = new(std::nothrow) VolumeSet;
	if (!fVolumes)
		return B_NO_MEMORY;
	error = fVolumes->InitCheck();
	if (error != B_OK)
		return error;

	// create node ID to volumes map
	fNodeIDs2Volumes = new(std::nothrow) NodeIDVolumeMap;
	if (!fNodeIDs2Volumes)
		return B_NO_MEMORY;
	error = fNodeIDs2Volumes->InitCheck();
	if (error != B_OK)
		return error;

	// create the volume event queue
	fVolumeEvents = new VolumeEventQueue;
	if (!fVolumeEvents)
		return B_NO_MEMORY;
	error = fVolumeEvents->InitCheck();
	if (error != B_OK)
		return error;

	// spawn the event deliverer
	#if USER
		fEventDeliverer = spawn_thread(&_EventDelivererEntry,
			"volume event deliverer", B_NORMAL_PRIORITY, this);
	#else
		fEventDeliverer = spawn_kernel_thread(&_EventDelivererEntry,
			"volume event deliverer", B_NORMAL_PRIORITY, this);
	#endif
	if (fEventDeliverer < 0)
		return fEventDeliverer;

	// create the root volume
	RootVolume* rootVolume = new(std::nothrow) RootVolume(this);
	if (!rootVolume)
		return B_NO_MEMORY;
	error = rootVolume->Init();
	if (error != B_OK) {
		delete rootVolume;
		return error;
	}
	fRootID = rootVolume->GetRootID();

	// add the root volume
	error = AddVolume(rootVolume);
	if (error != B_OK) {
		delete rootVolume;
		return error;
	}

	// mount the root volume
	error = rootVolume->Mount(device, fMountFlags, (const char*)parameters,
		len);
	if (error != B_OK) {
		rootVolume->SetUnmounting(true);
		PutVolume(rootVolume);
		return error;
	}
	rootVolume->AcquireReference();
	*volume = rootVolume;

	// run the event deliverer
	resume_thread(fEventDeliverer);

	return B_OK;
}

// UnmountRootVolume
void
VolumeManager::UnmountRootVolume()
{
	if (Volume* rootVolume = GetRootVolume()) {
		rootVolume->SetUnmounting(true);
		PutVolume(rootVolume);
	} else {
		ERROR(("VolumeManager::UnmountRootVolume(): ERROR: Couldn't get "
			"root volume!\n"));
	}
}

// GetQueryManager
QueryManager*
VolumeManager::GetQueryManager() const
{
	return fQueryManager;
}

// GetRootVolume
Volume*
VolumeManager::GetRootVolume()
{
	return GetVolume(fRootID);
}

// AddVolume
//
// The caller must have a reference to the volume and retains it.
status_t
VolumeManager::AddVolume(Volume* volume)
{
	if (!volume)
		return B_BAD_VALUE;

	// check, if it already exists
	AutoLocker<Locker> _(this);
	if (fVolumes->Contains(volume))
		return B_BAD_VALUE;

	// add the volume
	return fVolumes->Add(volume);
}

// GetVolume
Volume*
VolumeManager::GetVolume(vnode_id nodeID)
{
	AutoLocker<Locker> _(this);
	Volume* volume = fNodeIDs2Volumes->Get(nodeID);
	if (volume && GetVolume(volume))
		return volume;
	return NULL;
}

// GetVolume
Volume*
VolumeManager::GetVolume(Volume* volume)
{
	if (!volume)
		return NULL;

	AutoLocker<Locker> _(this);
	if (fVolumes->Contains(volume)) {
// TODO: Any restrictions regarding volumes about to be removed?
		volume->AcquireReference();
		return volume;
	}
	return NULL;
}

// PutVolume
//
// The VolumeManager must not be locked, when this method is invoked.
void
VolumeManager::PutVolume(Volume* volume)
{
	if (!volume)
		return;

	// If the volume is marked unmounting and is not yet marked removed, we
	// initiate the removal process.
	{
		AutoLocker<Locker> locker(this);
//PRINT(("VolumeManager::PutVolume(%p): reference count before: %ld\n",
//volume, volume->CountReferences()));
		if (volume->IsUnmounting() && !volume->IsRemoved()) {
//PRINT(("VolumeManager::PutVolume(%p): Volume connection broken, marking "
//"removed and removing all nodes.\n", volume));
			// mark removed
			volume->MarkRemoved();

			// get parent volume
			Volume* parentVolume = volume->GetParentVolume();
			if (parentVolume && !GetVolume(parentVolume))
				parentVolume = NULL;

			locker.Unlock();

			// prepare to unmount
			volume->PrepareToUnmount();

			// remove from parent volume
			if (parentVolume) {
				parentVolume->RemoveChildVolume(volume);
				PutVolume(parentVolume);
			}
		}
	}

	// If the volume is marked removed and it's reference count drops to 0,
	// we unmount and delete it.
	{
		AutoLocker<Locker> locker(this);
		if (volume->ReleaseReference() && volume->IsRemoved()) {
			PRINT("VolumeManager::PutVolume(%p): Removed volume "
				"unreferenced. Unmounting...\n", volume);
			// remove from volume set -- now noone can get a reference to it
			// anymore
			fVolumes->Remove(volume);

			locker.Unlock();

			// unmount and delete the volume
// TODO: At some point all the volume's node IDs have to be removed from
// fNodeIDs2Volumes. For the time being we expect the volume to do that itself
// in Unmount().
			volume->Unmount();
			delete volume;
		}
	}
}

// NewNodeID
vnode_id
VolumeManager::NewNodeID(Volume* volume)
{
	if (!volume)
		return B_BAD_VALUE;

	AutoLocker<Locker> _(this);
	vnode_id nodeID = fNextNodeID;
	status_t error = fNodeIDs2Volumes->Put(nodeID, volume);
	if (error != B_OK)
		return error;
	return fNextNodeID++;
}

// RemoveNodeID
void
VolumeManager::RemoveNodeID(vnode_id nodeID)
{
	AutoLocker<Locker> _(this);
	fNodeIDs2Volumes->Remove(nodeID);
}

// SendVolumeEvent
void
VolumeManager::SendVolumeEvent(VolumeEvent* event)
{
	if (!event)
		return;

	fVolumeEvents->Push(event);
}

// _EventDelivererEntry
int32
VolumeManager::_EventDelivererEntry(void* data)
{
	return ((VolumeManager*)data)->_EventDeliverer();
}

// _EventDeliverer
int32
VolumeManager::_EventDeliverer()
{
	while (VolumeEvent* event = fVolumeEvents->Pop()) {
		if (Volume* volume = GetVolume(event->GetTarget())) {
			volume->HandleEvent(event);
			PutVolume(volume);
		}
		event->ReleaseReference();
	}
	return 0;
}

