/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__BUFFER_CACHE_H_
#define _PACKAGE__HPKG__BUFFER_CACHE_H_


#include <stddef.h>

#include <util/DoublyLinkedList.h>


namespace BPackageKit {

namespace BHaikuPackage {

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


class BufferCache {
public:
	virtual						~BufferCache();

	virtual	CachedBuffer*		GetBuffer(size_t size,
									CachedBuffer** owner = NULL,
									bool* _newBuffer = NULL) = 0;
	virtual	void				PutBufferAndCache(CachedBuffer** owner) = 0;
									// caller is buffer owner and wants the
									// buffer cached, if possible
	virtual	void				PutBuffer(CachedBuffer** owner) = 0;
									// puts the buffer for good, owner might
									// have called PutBufferAndCache() before
									// and might not own a buffer anymore
};


class CachedBufferPutter {
public:
	CachedBufferPutter(BufferCache* cache, CachedBuffer** owner)
		:
		fCache(cache),
		fOwner(owner),
		fBuffer(NULL)
	{
	}

	CachedBufferPutter(BufferCache* cache, CachedBuffer* buffer)
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
	BufferCache*	fCache;
	CachedBuffer**	fOwner;
	CachedBuffer*	fBuffer;
};


}	// namespace BPrivate

}	// namespace BHaikuPackage

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__BUFFER_CACHE_H_
