// KernelDebug.cpp

#include "KernelDebug.h"

#include <KernelExport.h>

#include "Debug.h"
#include "FileSystem.h"
#include "FileSystemInitializer.h"
#include "RequestPort.h"
#include "RequestPortPool.h"
#include "UserlandFS.h"
#include "Volume.h"

static vint32 sCommandsAdded = 0;

// DebugUFS
int
KernelDebug::DebugUFS(int argc, char** argv)
{
	typedef HashMap<String, FileSystemInitializer*> KDebugFSMap;
	UserlandFS* userlandFS = UserlandFS::GetUserlandFS();
	KDebugFSMap& fileSystems = userlandFS->fFileSystems->GetUnsynchronizedMap();

	for (KDebugFSMap::Iterator it = fileSystems.GetIterator();
		 it.HasNext();) {
		KDebugFSMap::Entry entry = it.Next();
		FileSystemInitializer* fsInitializer = entry.value;
		FileSystem* fs = fsInitializer->GetFileSystem();
		kprintf("file system %p: %s\n", fs, (fs ? fs->GetName() : NULL));
		if (fs) {
			kprintf("  port pool %p\n", fs->GetPortPool());
			int32 volumeCount = fs->fVolumes.Count();
			for (int32 i = 0; i < volumeCount; i++) {
				Volume* volume = fs->fVolumes.ElementAt(i);
				kprintf("  volume %p: %ld\n", volume, volume->GetID());
			}
		}
	}
	return 0;
}

// DebugPortPool
int
KernelDebug::DebugPortPool(int argc, char** argv)
{
	if (argc < 2) {
		kprintf("usage: ufs_portpool <port pool pointer>\n");
		return 0;
	}
	RequestPortPool *portPool = (RequestPortPool*)parse_expression(argv[1]);
	kprintf("free ports:\n");
	for (int32 i = 0; i < portPool->fFreePorts; i++) {
		kprintf("  port %p\n", portPool->fPorts[i].port);
	}
	kprintf("used ports:\n");
	for (int32 i = portPool->fFreePorts; i < portPool->fPortCount; i++) {
		kprintf("  port %p, owner: %ld, count: %ld\n", portPool->fPorts[i].port,
			portPool->fPorts[i].owner, portPool->fPorts[i].count);
	}
	return 0;
}

// DebugPort
int
KernelDebug::DebugPort(int argc, char** argv)
{
	if (argc < 2) {
		kprintf("usage: ufs_port <port pointer>\n");
		return 0;
	}
	RequestPort *port = (RequestPort*)parse_expression(argv[1]);
	kprintf("port %p:\n", port);
	kprintf("  status      : %lx\n", port->fPort.fInitStatus);
	kprintf("  is owner    : %d\n", port->fPort.fOwner);
	kprintf("  owner port:   %ld\n", port->fPort.fInfo.owner_port);
	kprintf("  client port:  %ld\n", port->fPort.fInfo.client_port);
	kprintf("  size:         %ld\n", port->fPort.fInfo.size);
	kprintf("  capacity:     %ld\n", port->fPort.fCapacity);
	kprintf("  buffer:       %p\n", port->fPort.fBuffer);
	return 0;
}

// #pragma mark -

// AddDebuggerCommands
void
KernelDebug::AddDebuggerCommands()
{
	if (atomic_add(&sCommandsAdded, 1) > 0)
		return;
	PRINT(("KernelDebug::AddDebuggerCommands(): adding debugger commands\n"));
	add_debugger_command("ufs", DebugUFS, "prints general info about "
		"userland FS");
	add_debugger_command("ufs_portpool", DebugPortPool,
		"ufs_portpool <port pool pointer> - prints info about a "
		"userland FS port pool");
	add_debugger_command("ufs_port", DebugPort,
		"ufs_port <port pointer> - prints info about a userland FS port");
}

// RemoveDebuggerCommands
void
KernelDebug::RemoveDebuggerCommands()
{
	if (atomic_add(&sCommandsAdded, -1) > 1)
		return;
	PRINT(("KernelDebug::RemoveDebuggerCommands(): removing debugger "
		"commands\n"));
	remove_debugger_command("ufs_port", DebugPort);
	remove_debugger_command("ufs_portpool", DebugPortPool);
	remove_debugger_command("ufs", DebugUFS);
}

