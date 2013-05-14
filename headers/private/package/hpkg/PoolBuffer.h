/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__POOL_BUFFER_H_
#define _PACKAGE__HPKG__PRIVATE__POOL_BUFFER_H_


#include <stddef.h>

#include <util/DoublyLinkedList.h>

#include <package/hpkg/BufferPool.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


class PoolBuffer : public DoublyLinkedListLinkImpl<PoolBuffer> {
public:
								PoolBuffer(size_t size);
								~PoolBuffer();

			void*				Buffer() const	{ return fBuffer; }
			size_t				Size() const	{ return fSize; }


			// implementation private
			PoolBuffer**		Owner() const	{ return fOwner; }
			void				SetOwner(PoolBuffer** owner)
									{ fOwner = owner; }

			void				SetCached(bool cached)	{ fCached = cached; }
			bool				IsCached() const		{ return fCached; }

private:
			PoolBuffer**		fOwner;
			void*				fBuffer;
			size_t				fSize;
			bool				fCached;
};


class PoolBufferPutter {
public:
	PoolBufferPutter(BBufferPool* pool, PoolBuffer** owner)
		:
		fPool(pool),
		fOwner(owner),
		fBuffer(NULL)
	{
	}

	PoolBufferPutter(BBufferPool* pool, PoolBuffer* buffer)
		:
		fPool(pool),
		fOwner(NULL),
		fBuffer(buffer)
	{
	}

	~PoolBufferPutter()
	{
		if (fPool != NULL) {
			if (fOwner != NULL)
				fPool->PutBufferAndCache(fOwner);
			else if (fBuffer != NULL)
				fPool->PutBuffer(&fBuffer);
		}
	}

private:
	BBufferPool*	fPool;
	PoolBuffer**	fOwner;
	PoolBuffer*		fBuffer;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__POOL_BUFFER_H_
