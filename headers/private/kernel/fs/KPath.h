// KPath.h
//
// A simple class wrapping a path. Has a fixed-sized buffer.

#ifndef _K_PATH_H
#define _K_PATH_H

#include <KernelExport.h>

namespace BPrivate {
namespace DiskDevice {

class KPath {
public:
	KPath(int32 bufferSize = B_PATH_NAME_LENGTH);
	KPath(const char* path, bool normalize = false,
		int32 bufferSize = B_PATH_NAME_LENGTH);
	KPath(const KPath& other);
	~KPath();

	status_t SetTo(const char *path, bool normalize = false,
		int32 bufferSize = B_PATH_NAME_LENGTH);

	status_t InitCheck() const;

	status_t SetPath(const char *path, bool normalize = false);
	const char *Path() const;
	int32 Length() const;

	int32 BufferSize() const;
	char *LockBuffer();
	void UnlockBuffer();

	const char *Leaf() const;
	status_t ReplaceLeaf(const char *newLeaf);

	status_t Append(const char *toAppend, bool isComponent = true);

	KPath& operator=(const KPath& other);
	KPath& operator=(const char* path);

	bool operator==(const KPath& other) const;
	bool operator==(const char* path) const;
	bool operator!=(const KPath& other) const;
	bool operator!=(const char* path) const;

private:
	void _ChopTrailingSlashes();

	char*	fBuffer;
	int32	fBufferSize;
	int32	fPathLength;
	bool	fLocked;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KPath;

#endif _K_PATH_H
