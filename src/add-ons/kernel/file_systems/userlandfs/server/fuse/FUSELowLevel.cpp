/*
 * Copyright 2022, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Distributed under terms of the MIT license.
 */

#include "FUSELowLevel.h"

#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include <Errors.h>

#define ROUNDDOWN(a, b) (((a) / (b)) * (b))
#define ROUNDUP(a, b)   ROUNDDOWN((a) + (b) - 1, b)



// Reimplement fuse_req in our own way. In libfuse, the requests are communicated to the kernel,
// but in our case, they remain entirely inside userlandfs_server. This means we can use a much
// simpler system, by passing pointers directly between userlandfs and the libfuse client code,
// and synchronizing them with a semaphore to wait for the replies.
struct fuse_req {
	fuse_req()
		: fReplyResult(0),
		fReplyBuf(NULL)
	{
		sem_init(&fSyncSem, 0, 0);
	}

	~fuse_req() {
		sem_destroy(&fSyncSem);
	}

	void Wait() {
		sem_wait(&fSyncSem);
	}

	void Notify() {
		sem_post(&fSyncSem);
	}

	sem_t fSyncSem;
	ssize_t fReplyResult;

	ReadDirBufferFiller fRequestFiller;
	void* fRequestCookie;

	// The reply can contain various things, depending on which function was called
	union {
		struct stat* fReplyAttr;
		struct fuse_entry_param fReplyEntry;
		struct fuse_file_info* fReplyOpen;
		struct statvfs* fReplyStat;

		char* fReplyBuf;
	};
};


// #pragma mark - Convenience functions for calling fuse lowlevel filesstems easily


void
fuse_ll_init(const fuse_lowlevel_ops* ops, void* userdata, struct fuse_conn_info* conn)
{
	if (ops->init != NULL) {
		ops->init(userdata, conn);
	}
}


void
fuse_ll_destroy(const fuse_lowlevel_ops* ops, void* userdata)
{
	if (ops->destroy != NULL) {
		ops->destroy(userdata);
	}
}


int
fuse_ll_lookup(const fuse_lowlevel_ops* ops, fuse_ino_t parent, const char *name,
	struct stat* st)
{
	if (ops->lookup == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	ops->lookup(&request, parent, name);
	request.Wait();
	*st = request.fReplyEntry.attr;
	st->st_ino = request.fReplyEntry.ino;
	return request.fReplyResult;
}


int
fuse_ll_getattr(const fuse_lowlevel_ops* ops, fuse_ino_t ino, struct stat* st)
{
	if (ops->getattr == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	request.fReplyAttr = st;
	ops->getattr(&request, ino, NULL);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_setattr(const fuse_lowlevel_ops* ops, fuse_ino_t ino, const struct stat *attr,
	int to_set)
{
	if (ops->setattr == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	//request.fReplyAttr = attr;
	ops->setattr(&request, ino, const_cast<struct stat*>(attr), to_set, NULL);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_readlink(const fuse_lowlevel_ops* ops, fuse_ino_t ino, char* buffer, size_t size)
{
	if (ops->readlink == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	request.fReplyBuf = buffer;
	ops->readlink(&request, ino);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_mkdir(const fuse_lowlevel_ops* ops, fuse_ino_t parent, const char *name,
	mode_t mode)
{
	if (ops->mkdir == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	ops->mkdir(&request, parent, name, mode);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_unlink(const fuse_lowlevel_ops* ops, fuse_ino_t parent, const char *name)
{
	if (ops->unlink == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	ops->unlink(&request, parent, name);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_rmdir(const fuse_lowlevel_ops* ops, fuse_ino_t parent, const char *name)
{
	if (ops->rmdir == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	ops->rmdir(&request, parent, name);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_symlink(const fuse_lowlevel_ops* ops, const char* link, fuse_ino_t parent,
	const char* name)
{
	if (ops->symlink == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	ops->symlink(&request, link, parent, name);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_rename(const fuse_lowlevel_ops* ops, fuse_ino_t parent, const char *name,
	fuse_ino_t newparent, const char *newname)
{
	if (ops->rename == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	ops->rename(&request, parent, name, newparent, newname);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_link(const fuse_lowlevel_ops* ops, fuse_ino_t ino, fuse_ino_t newparent,
	const char *newname)
{
	if (ops->link == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	ops->link(&request, ino, newparent, newname);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_open(const fuse_lowlevel_ops* ops, fuse_ino_t ino, fuse_file_info* ffi)
{
	if (ops->open == NULL)
		return 0;

	fuse_req request;
	request.fReplyOpen = ffi;
	ops->open(&request, ino, ffi);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_read(const fuse_lowlevel_ops* ops, fuse_ino_t ino, char* buffer, size_t bufferSize,
	off_t position, fuse_file_info* ffi)
{
	if (ops->read == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	request.fReplyBuf = buffer;
	ops->read(&request, ino, bufferSize, position, ffi);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_write(const fuse_lowlevel_ops* ops, fuse_ino_t ino, const char *buf,
	size_t size, off_t off, struct fuse_file_info *fi)
{
	if (ops->write == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	ops->write(&request, ino, buf, size, off, fi);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_flush(const fuse_lowlevel_ops* ops, fuse_ino_t ino, fuse_file_info* ffi)
{
	if (ops->flush == NULL)
		return 0;

	fuse_req request;
	ops->flush(&request, ino, ffi);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_release(const fuse_lowlevel_ops* ops, fuse_ino_t ino, fuse_file_info* ffi)
{
	if (ops->release == NULL)
		return 0;

	fuse_req request;
	ops->release(&request, ino, ffi);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_fsync(const fuse_lowlevel_ops* ops, fuse_ino_t ino, int datasync, fuse_file_info* ffi)
{
	if (ops->fsync == NULL)
		return 0;

	fuse_req request;
	ops->fsync(&request, ino, datasync, ffi);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_opendir(const fuse_lowlevel_ops* ops, fuse_ino_t inode, struct fuse_file_info* ffi)
{
	// intentioanlly check for readdir here. Some filesystems do not need an opendir, but still
	// implement readdir. However if readdir is not implemented, there is no point in trying to
	// open a directory.
	if (ops->readdir == NULL)
		return B_NOT_SUPPORTED;

	if (ops->opendir) {
		fuse_req request;
		ops->opendir(&request, inode, ffi);
		request.Wait();
		return request.fReplyResult;
	}

	return 0;
}


int
fuse_ll_readdir(const fuse_lowlevel_ops* ops, fuse_ino_t ino, void* cookie, char* buffer,
	size_t bufferSize, ReadDirBufferFiller filler, off_t pos, fuse_file_info* ffi)
{
	if (ops->readdir == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	request.fReplyBuf = buffer;

	request.fRequestFiller = filler;
	request.fRequestCookie = cookie;

	ops->readdir(&request, ino, bufferSize, pos, ffi);

	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_releasedir(const fuse_lowlevel_ops* ops, fuse_ino_t ino, struct fuse_file_info *fi)
{
	if (ops->releasedir == NULL)
		return 0;

	fuse_req request;
	ops->releasedir(&request, ino, fi);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_statfs(const fuse_lowlevel_ops* ops, fuse_ino_t inode, struct statvfs* stat)
{
	if (ops->statfs == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	request.fReplyStat = stat;
	ops->statfs(&request, inode);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_getxattr(const fuse_lowlevel_ops* ops, fuse_ino_t ino, const char *name,
	char* buffer, size_t size)
{
	if (ops->getxattr == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	request.fReplyBuf = buffer;
	ops->getxattr(&request, ino, name, size);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_listxattr(const fuse_lowlevel_ops* ops, fuse_ino_t ino, char* buffer, size_t size)
{
	if (ops->listxattr == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	request.fReplyBuf = (char*)buffer;
	ops->listxattr(&request, ino, size);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_access(const fuse_lowlevel_ops* ops, fuse_ino_t ino, int mask)
{
	if (ops->access == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	ops->access(&request, ino, mask);
	request.Wait();
	return request.fReplyResult;
}


int
fuse_ll_create(const fuse_lowlevel_ops* ops, fuse_ino_t parent, const char *name,
	mode_t mode, struct fuse_file_info *fi, fuse_ino_t& ino)
{
	// TODO if the create op is missing, we could try using mknod + open instead
	if (ops->create == NULL)
		return B_NOT_SUPPORTED;

	fuse_req request;
	ops->create(&request, parent, name, mode, fi);
	request.Wait();
	ino = request.fReplyEntry.ino;
	return request.fReplyResult;
}


//#pragma mark - lowlevel replies handling


int
fuse_reply_attr(fuse_req_t req, const struct stat *attr, double attr_timeout)
{
	*req->fReplyAttr = *attr;
	req->Notify();
	return 0;
}


int
fuse_reply_create(fuse_req_t req, const struct fuse_entry_param* e, const struct fuse_file_info* fi)
{
	req->fReplyEntry = *e;
	req->Notify();
	return 0;
}


int
fuse_reply_readlink(fuse_req_t req, const char* link)
{
	strlcpy(req->fReplyBuf, link, req->fReplyResult);
	req->fReplyResult = strlen(link);
	req->Notify();
	return 0;
}


int
fuse_reply_open(fuse_req_t req, const struct fuse_file_info* f)
{
	*req->fReplyOpen = *f;
	req->Notify();
	return 0;
}


int
fuse_reply_buf(fuse_req_t req, const char *buf, size_t size)
{
	if (req->fReplyBuf && req->fReplyBuf != buf)
		memcpy(req->fReplyBuf, buf, size);

	req->fReplyResult = size;

	req->Notify();
	return 0;
}


int
fuse_reply_entry(fuse_req_t req, const struct fuse_entry_param *e)
{
	req->fReplyEntry = *e;
	req->Notify();
	return 0;
}


int
fuse_reply_err(fuse_req_t req, int err)
{
	assert(err >= 0);
	req->fReplyResult = -err;
	req->Notify();
	return 0;
}


int
fuse_reply_statfs(fuse_req_t req, const struct statvfs* stat)
{
	*req->fReplyStat = *stat;
	req->Notify();
	return 0;
}


int
fuse_reply_write(fuse_req_t req, size_t count)
{
	req->fReplyResult = count;
	req->Notify();
	return 0;
}


// return: size of the entry (no matter if it was added to the buffer or not)
// params: pointer to where to store the entry, size
size_t fuse_add_direntry(fuse_req_t req, char *buf, size_t bufsize, const char *name,
	const struct stat *stbuf, off_t off)
{
	size_t entryLen = offsetof(struct dirent, d_name) + strlen(name) + 1;
	// align the entry length, so the next dirent will be aligned
	entryLen = ROUNDUP(entryLen, 8);

	if (stbuf != NULL) {
		req->fRequestFiller(req->fRequestCookie, buf, bufsize, name, stbuf, off);
	}

	return entryLen;
}


// #pragma mark - Stubs for FUSE functions called by client code, that we don't need


void fuse_session_add_chan(struct fuse_session* se, struct fuse_chan* ch)
{
}

void fuse_session_remove_chan(struct fuse_chan* ch)
{
}

void fuse_session_destroy(struct fuse_session* se)
{
}

void fuse_session_exit(struct fuse_session* se)
{
}
