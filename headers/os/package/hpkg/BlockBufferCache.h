/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__BLOCK_BUFFER_CACHE_H_
#define _PACKAGE__HPKG__BLOCK_BUFFER_CACHE_H_


#include <SupportDefs.h>

#include <package/hpkg/BufferCache.h>


namespace BPackageKit {

namespace BHPKG {


namespace BPrivate {
	class BlockBufferCacheImpl;
}
using BPrivate::BlockBufferCacheImpl;


class BBlockBufferCache : public BBufferCache, public BBufferCacheLockable {
public:
								BBlockBufferCache(size_t blockSize,
									uint32 maxCachedBlocks);
	virtual						~BBlockBufferCache();

	virtual	status_t			Init();

	virtual	CachedBuffer*		GetBuffer(size_t size,
									CachedBuffer** owner = NULL,
									bool* _newBuffer = NULL);
	virtual	void				PutBufferAndCache(CachedBuffer** owner);
	virtual	void				PutBuffer(CachedBuffer** owner);

private:
			BlockBufferCacheImpl*	fImpl;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__BLOCK_BUFFER_CACHE_H_
