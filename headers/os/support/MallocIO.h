// MallocIO.h
// Just here to be able to compile and test BResources.
// To be replaced by the OpenBeOS version to be provided by the IK Team.

#ifndef _sk_malloc_io_h
#define _sk_malloc_io_h

#include <DataIO.h>

class BMallocIO : public BPositionIO {
public:
	BMallocIO();
	virtual ~BMallocIO();

	virtual ssize_t ReadAt(off_t pos, void *buffer, size_t size);
	virtual ssize_t WriteAt(off_t pos, const void *buffer, size_t size);

	virtual off_t Seek(off_t pos, uint32 seekMode);
	virtual off_t Position() const;
	virtual status_t SetSize(off_t size);

	void SetBlockSize(size_t blockSize);

	const void *Buffer() const;
	size_t BufferLength() const;

private:
	status_t _Resize(off_t size);

	virtual void _ReservedMallocIO1();
	virtual void _ReservedMallocIO2();

	BMallocIO(const BMallocIO &);
	BMallocIO &operator=(const BMallocIO &);

private:
	size_t	fBlockSize;
	size_t	fMallocSize;
	size_t	fLength;
	char	*fData;
	off_t	fPosition;
	int32	_reserved[1];
};

#endif	// _sk_malloc_io_h
