/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "FUSEFileSystem.h"

#include <stdlib.h>
#include <string.h>

#include <new>

#include "fuse_fs.h"
#include "FUSEVolume.h"

#include "../RequestThread.h"


class FUSEFileSystem::ArgumentVector {
private:
	enum { MAX_ARGUMENTS = 128 };

public:
	ArgumentVector()
		:
		fBuffer(NULL),
		fCount(0)
	{
	}

	~ArgumentVector()
	{
		free(fBuffer);
	}

	const char* const* Arguments() const
	{
		return fArguments;
	}

	int ArgumentCount() const
	{
		return fCount;
	}

	status_t Init(const char* firstElement, const char* arguments)
	{
		size_t firstElementSize = firstElement != NULL
			? strlen(firstElement) + 1 : 0;

		// allocate the buffer
		fBuffer = (char*)malloc(firstElementSize + strlen(arguments) + 1);
		if (fBuffer == NULL)
			return B_NO_MEMORY;

		fCount = 0;

		bool inArgument = false;
		int bufferIndex = 0;

		// push the first element, if given
		if (firstElement != NULL) {
			memcpy(fBuffer, firstElement, firstElementSize);
			fArguments[fCount++] = fBuffer;
			bufferIndex = firstElementSize;
		}

		// parse the given string
		for (; *arguments != '\0'; arguments++) {
			char c = *arguments;
			switch (c) {
				case ' ':
				case '\t':
				case '\r':
				case '\n':
					// white-space marks argument boundaries
					if (inArgument) {
						// terminate the current argument
						fBuffer[bufferIndex++] = '\0';
						inArgument = false;
					}
					break;
				case '\\':
					c = *++arguments;
					if (c == '\0')
						break;
					// fall through
				default:
					if (!inArgument) {
						// push a new argument
						if (fCount == MAX_ARGUMENTS)
							break;

						fArguments[fCount++] = fBuffer + bufferIndex;
						inArgument = true;
					}

					fBuffer[bufferIndex++] = c;
					break;
			}
		}

		// terminate the last argument
		if (inArgument)
			fBuffer[bufferIndex++] = '\0';

		// NULL terminate the argument array
		fArguments[fCount] = NULL;

		return B_OK;
	}

private:
	char*		fBuffer;
	const char*	fArguments[MAX_ARGUMENTS + 1];
	int			fCount;
};


FUSEFileSystem::FUSEFileSystem(const char* fsName,
	int (*mainFunction)(int, const char* const*))
	:
	FileSystem(fsName),
	fMainFunction(mainFunction),
	fInitThread(-1),
	fInitStatus(B_NO_INIT),
	fInitSemaphore(-1),
	fExitSemaphore(-1),
	fInitParameters(NULL),
	fFS(NULL)
{
	fClientFSType = CLIENT_FS_FUSE;

	// FS capabilities
	fCapabilities.ClearAll();
	fCapabilities.Set(FS_CAPABILITY_MOUNT, true);
}


FUSEFileSystem::~FUSEFileSystem()
{
	if (fInitSemaphore >= 0)
		delete_sem(fInitSemaphore);

	if (fExitSemaphore >= 0)
		delete_sem(fExitSemaphore);

	if (fInitThread >= 0)
		wait_for_thread(fInitThread, NULL);
}


status_t
FUSEFileSystem::CreateVolume(Volume** _volume, dev_t id)
{
printf("FUSEFileSystem::CreateVolume()\n");
	// Only one volume is possible
	if (!fVolumes.IsEmpty())
		RETURN_ERROR(B_BUSY);

	// create the volume
	FUSEVolume* volume = new(std::nothrow) FUSEVolume(this, id);
	if (volume == NULL)
		return B_NO_MEMORY;

	status_t error = volume->Init();
	if (error != B_OK) {
		delete volume;
		return error;
	}

	*_volume = volume;
	return B_OK;
}


status_t
FUSEFileSystem::DeleteVolume(Volume* volume)
{
	delete volume;
	return B_OK;
}


void
FUSEFileSystem::InitRequestThreadContext(RequestThreadContext* context)
{
	// Statically assert that fuse_context fits in the RequestThreadContext
	// FS data. We can't include <Debug.h> as it clashes with our "Debug.h".
	do {
		static const int staticAssertHolds
			= sizeof(fuse_context) <= REQUEST_THREAD_CONTEXT_FS_DATA_SIZE;
		struct __staticAssertStruct__ {
			char __static_assert_failed__[2 * staticAssertHolds - 1];
		};
	} while (false);

	// init a fuse_context
	KernelRequest* request = context->GetRequest();
	fuse_context* fuseContext = (fuse_context*)context->GetFSData();
	fuseContext->fuse = (struct fuse*)this;
	fuseContext->uid = request->user;
	fuseContext->gid = request->group;
	fuseContext->pid = request->team;
	fuseContext->private_data = fFS != NULL ? fFS->userData : NULL;
}


status_t
FUSEFileSystem::InitClientFS(const char* parameters)
{
PRINT(("FUSEFileSystem::InitClientFS()\n"));
	// create the semaphores we need
	fInitSemaphore = create_sem(0, "FUSE init sem");
	if (fInitSemaphore < 0)
		RETURN_ERROR(fInitSemaphore);

	fExitSemaphore = create_sem(0, "FUSE exit sem");
	if (fExitSemaphore < 0)
		RETURN_ERROR(fExitSemaphore);

	fInitStatus = 1;
	fInitParameters = parameters;

	// Start the initialization thread -- it will call main() and won't return
	// until unmounting.
	fInitThread = spawn_thread(&_InitializationThreadEntry,
		"FUSE init", B_NORMAL_PRIORITY, this);
	if (fInitThread < 0)
		RETURN_ERROR(fInitThread);

	resume_thread(fInitThread);

	// wait for the initialization to finish
PRINT(("  waiting for init thread...\n"));
	while (acquire_sem(fInitSemaphore) == B_INTERRUPTED) {
	}

PRINT(("  waiting for init thread done\n"));
	fInitSemaphore = -1;

	if (fInitStatus > 0)
		RETURN_ERROR(B_ERROR);
	if (fInitStatus != B_OK)
		RETURN_ERROR(fInitStatus);

	// initialization went fine
	return B_OK;
}


void
FUSEFileSystem::ExitClientFS(status_t status)
{
	// set the exit status and notify the initialization thread
	fExitStatus = status;
	if (fExitSemaphore >= 0)
		delete_sem(fExitSemaphore);

	if (fInitThread >= 0)
		wait_for_thread(fInitThread, NULL);
}


status_t
FUSEFileSystem::FinishInitClientFS(fuse_config* config,
	const fuse_operations* ops, size_t opSize, void* userData)
{
PRINT(("FUSEFileSystem::FinishInitClientFS()\n"));
	fExitStatus = B_ERROR;

	fFUSEConfig = *config;

	// do the initialization
	fInitStatus = _InitClientFS(ops, opSize, userData);
	return fInitStatus;
}


status_t
FUSEFileSystem::MainLoop(bool multithreaded)
{
	// TODO: Respect the multithreaded flag!

PRINT(("FUSEFileSystem::FinishMounting()\n"));
	// notify the mount thread
PRINT(("  notifying mount thread\n"));
	delete_sem(fInitSemaphore);

	// loop until unmounting
PRINT(("  waiting for unmounting...\n"));
	while (acquire_sem(fExitSemaphore) == B_INTERRUPTED) {
	}
PRINT(("  waiting for unmounting done\n"));

	fExitSemaphore = -1;

	if (fFS != NULL)
		fuse_fs_destroy(fFS);

	return fExitStatus;
}


/*static*/ status_t
FUSEFileSystem::_InitializationThreadEntry(void* data)
{
	return ((FUSEFileSystem*)data)->_InitializationThread();
}


status_t
FUSEFileSystem::_InitializationThread()
{
	// parse the parameters
	ArgumentVector args;
	status_t error = args.Init(GetName(), fInitParameters);
	if (error != B_OK) {
		fInitStatus = error;
		delete_sem(fInitSemaphore);
		return B_OK;
	}

	// call main -- should not return until unmounting
	fMainFunction(args.ArgumentCount(), args.Arguments());
printf("FUSEFileSystem::_InitializationThread(): main() returned!\n");

	if (fInitStatus > 0 && fInitSemaphore >= 0) {
		// something went wrong early -- main() returned without calling
		// fuse_main()
		fInitStatus = B_ERROR;
		delete_sem(fInitSemaphore);
	}

	return B_OK;
}


status_t
FUSEFileSystem::_InitClientFS(const fuse_operations* ops, size_t opSize,
	void* userData)
{
	// create a fuse_fs object
	fFS = fuse_fs_new(ops, opSize, userData);
	if (fFS == NULL)
		return B_ERROR;

	_InitCapabilities();
PRINT(("volume capabilities:\n"));
fVolumeCapabilities.Dump();
PRINT(("node capabilities:\n"));
fNodeCapabilities.Dump();

	// init connection info
	fConnectionInfo.proto_major = 0;
	fConnectionInfo.proto_minor = 0;
	fConnectionInfo.async_read = false;
	fConnectionInfo.max_write = 64 * 1024;
	fConnectionInfo.max_readahead = 64 * 1024;

	fuse_fs_init(fFS, &fConnectionInfo);

	return B_OK;
}


void
FUSEFileSystem::_InitCapabilities()
{
	fVolumeCapabilities.ClearAll();
	fNodeCapabilities.ClearAll();

	// Volume operations
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_UNMOUNT, true);

	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_READ_FS_INFO, fFS->ops.statfs);
	// missing: FS_VOLUME_CAPABILITY_WRITE_FS_INFO
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_SYNC, fFS->ops.fsync);
		// emulated via fsync()

	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_GET_VNODE, true);
		// emulated

	// index directory & index operations
	// missing: FS_VOLUME_CAPABILITY_OPEN_INDEX_DIR
	// missing: FS_VOLUME_CAPABILITY_CLOSE_INDEX_DIR
	// missing: FS_VOLUME_CAPABILITY_FREE_INDEX_DIR_COOKIE
	// missing: FS_VOLUME_CAPABILITY_READ_INDEX_DIR
	// missing: FS_VOLUME_CAPABILITY_REWIND_INDEX_DIR

	// missing: FS_VOLUME_CAPABILITY_CREATE_INDEX
	// missing: FS_VOLUME_CAPABILITY_REMOVE_INDEX
	// missing: FS_VOLUME_CAPABILITY_READ_INDEX_STAT

	// query operations
	// missing: FS_VOLUME_CAPABILITY_OPEN_QUERY
	// missing: FS_VOLUME_CAPABILITY_CLOSE_QUERY
	// missing: FS_VOLUME_CAPABILITY_FREE_QUERY_COOKIE
	// missing: FS_VOLUME_CAPABILITY_READ_QUERY
	// missing: FS_VOLUME_CAPABILITY_REWIND_QUERY

	// vnode operations
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_LOOKUP, true);
		// emulated
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_GET_VNODE_NAME, true);
		// emulated
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_PUT_VNODE, true);
		// emulated
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_REMOVE_VNODE, true);
		// emulated

	// VM file access
	// missing: FS_VNODE_CAPABILITY_CAN_PAGE
	// missing: FS_VNODE_CAPABILITY_READ_PAGES
	// missing: FS_VNODE_CAPABILITY_WRITE_PAGES

	// cache file access
	// missing: FS_VNODE_CAPABILITY_GET_FILE_MAP

	// common operations
	// missing: FS_VNODE_CAPABILITY_IOCTL
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_SET_FLAGS, true);
		// emulated
	// missing: FS_VNODE_CAPABILITY_SELECT
	// missing: FS_VNODE_CAPABILITY_DESELECT
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_FSYNC, fFS->ops.fsync);

	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_READ_SYMLINK, fFS->ops.readlink);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_CREATE_SYMLINK, fFS->ops.symlink);

	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_LINK, fFS->ops.link);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_UNLINK, fFS->ops.unlink);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_RENAME, fFS->ops.rename);

	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_ACCESS, fFS->ops.access);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_READ_STAT, fFS->ops.getattr);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_WRITE_STAT,
		fFS->ops.chmod != NULL || fFS->ops.chown != NULL
		|| fFS->ops.truncate != NULL || fFS->ops.utimens != NULL
		|| fFS->ops.utime != NULL);

	// file operations
 	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_CREATE, fFS->ops.create);
 	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_OPEN, fFS->ops.open);
 	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_CLOSE, fFS->ops.flush);
 	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_FREE_COOKIE, fFS->ops.release);
 	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_READ, fFS->ops.read);
 	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_WRITE, fFS->ops.write);

	// directory operations
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_CREATE_DIR, fFS->ops.mkdir);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_REMOVE_DIR, fFS->ops.rmdir);
	bool readDirSupport = fFS->ops.opendir != NULL || fFS->ops.readdir != NULL
		|| fFS->ops.getdir;
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_OPEN_DIR, readDirSupport);
	// not needed: FS_VNODE_CAPABILITY_CLOSE_DIR
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_FREE_DIR_COOKIE, readDirSupport);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_READ_DIR, readDirSupport);
 	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_REWIND_DIR, readDirSupport);

	// attribute directory operations
	bool hasAttributes = fFS->ops.listxattr != NULL;
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_OPEN_ATTR_DIR, hasAttributes);
	// not needed: FS_VNODE_CAPABILITY_CLOSE_ATTR_DIR
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_FREE_ATTR_DIR_COOKIE,
		hasAttributes);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_READ_ATTR_DIR, hasAttributes);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_REWIND_ATTR_DIR, hasAttributes);

	// attribute operations
// 	// we emulate open_attr() and free_attr_dir_cookie() if either read_attr()
// 	// or write_attr() is present
// 	bool hasAttributes = (fFS->ops.read_attr || fFS->ops.write_attr);
// 	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_CREATE_ATTR, hasAttributes);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_OPEN_ATTR, hasAttributes);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_CLOSE_ATTR, false);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_FREE_ATTR_COOKIE, hasAttributes);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_READ_ATTR, fFS->ops.getxattr);
// 	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_WRITE_ATTR, fFS->ops.write_attr);

	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_READ_ATTR_STAT,
		fFS->ops.getxattr);
// 	// missing: FS_VNODE_CAPABILITY_WRITE_ATTR_STAT
// 	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_RENAME_ATTR, fFS->ops.rename_attr);
// 	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_REMOVE_ATTR, fFS->ops.remove_attr);
}


// #pragma mark - bootstrapping


status_t
userlandfs_create_file_system(const char* fsName, image_id image,
	FileSystem** _fileSystem)
{
printf("userlandfs_create_file_system()\n");
	// look up the main() function of the add-on
	int (*mainFunction)(int argc, const char* const* argv);
	status_t error = get_image_symbol(image, "main", B_SYMBOL_TYPE_TEXT,
		(void**)&mainFunction);
	if (error != B_OK)
		return error;
printf("userlandfs_create_file_system(): found main: %p\n", mainFunction);

	// create the file system
	FUSEFileSystem* fileSystem = new(std::nothrow) FUSEFileSystem(fsName,
		mainFunction);
	if (fileSystem == NULL)
		return B_NO_MEMORY;

	*_fileSystem = fileSystem;
	return B_OK;
}
