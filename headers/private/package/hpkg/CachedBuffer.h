/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__BUFFER_CACHE_H_
#define _PACKAGE__HPKG__PRIVATE__BUFFER_CACHE_H_


#include <stddef.h>

#include <util/DoublyLinkedList.h>

#include <package/hpkg/BufferCache.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


class CachedBuffer : public DoublyLinkedListLinkImpl<CachedBuffer> {
public:
								CachedBuffer(size_t size);
								~CachedBuffer();

			void*				Buffer() const	{ return fBuffer; }
			size_t				Size() const	{ return fSize; }


			// implementation private
			CachedBuffer**		Owner() const	{ return fOwner; }
			void				SetOwner(CachedBuffer** owner)
									{ fOwner = owner; }

			void				SetCached(bool cached)	{ fCached = cached; }
			bool				IsCached() const		{ return fCached; }

private:
			CachedBuffer**		fOwner;
			void*				fBuffer;
			size_t				fSize;
			bool				fCached;
};


class CachedBufferPutter {
public:
	CachedBufferPutter(BBufferCache* cache, CachedBuffer** owner)
		:
		fCache(cache),
		fOwner(owner),
		fBuffer(NULL)
	{
	}

	CachedBufferPutter(BBufferCache* cache, CachedBuffer* buffer)
		:
		fCache(cache),
		fOwner(NULL),
		fBuffer(buffer)
	{
	}

	~CachedBufferPutter()
	{
		if (fCache != NULL) {
			if (fOwner != NULL)
				fCache->PutBufferAndCache(fOwner);
			else if (fBuffer != NULL)
				fCache->PutBuffer(&fBuffer);
		}
	}

private:
	BBufferCache*	fCache;
	CachedBuffer**	fOwner;
	CachedBuffer*	fBuffer;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__BUFFER_CACHE_H_
