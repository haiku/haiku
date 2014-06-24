/*
 * Copyright 2013-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef HEAP_CACHE_H
#define HEAP_CACHE_H


#include <package/hpkg/DataReader.h>

#include <condition_variable.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>
#include <vm/vm_types.h>


using BPackageKit::BHPKG::BAbstractBufferedDataReader;
using BPackageKit::BHPKG::BDataReader;


class CachedDataReader : public BAbstractBufferedDataReader {
public:
								CachedDataReader();
	virtual						~CachedDataReader();

			status_t			Init(BAbstractBufferedDataReader* reader,
									off_t size);

	virtual	status_t			ReadDataToOutput(off_t offset, size_t size,
									BDataIO* output);

private:
			class CacheLineLocker
				: public DoublyLinkedListLinkImpl<CacheLineLocker> {
			public:
				CacheLineLocker(CachedDataReader* reader, off_t cacheLineOffset)
					:
					fReader(reader),
					fOffset(cacheLineOffset)
				{
					fReader->_LockCacheLine(this);
				}

				~CacheLineLocker()
				{
					fReader->_UnlockCacheLine(this);
				}

				off_t Offset() const
				{
					return fOffset;
				}

				CacheLineLocker*& HashNext()
				{
					return fHashNext;
				}

				DoublyLinkedList<CacheLineLocker>& Queue()
				{
					return fQueue;
				}

				void Wait(mutex& lock)
				{
					fWaitCondition.Init(this, "cached reader line locker");
					ConditionVariableEntry waitEntry;
					fWaitCondition.Add(&waitEntry);
					mutex_unlock(&lock);
					waitEntry.Wait();
					mutex_lock(&lock);
				}

				void WakeUp()
				{
					fWaitCondition.NotifyOne();
				}

			private:
				CachedDataReader*					fReader;
				off_t								fOffset;
				CacheLineLocker*					fHashNext;
				DoublyLinkedList<CacheLineLocker>	fQueue;
				ConditionVariable					fWaitCondition;
			};

			friend class CacheLineLocker;

			struct LockerHashDefinition {
				typedef off_t			KeyType;
				typedef	CacheLineLocker	ValueType;

				size_t HashKey(off_t key) const
				{
					return size_t(key / kCacheLineSize);
				}

				size_t Hash(const CacheLineLocker* value) const
				{
					return HashKey(value->Offset());
				}

				bool Compare(off_t key, const CacheLineLocker* value) const
				{
					return value->Offset() == key;
				}

				CacheLineLocker*& GetLink(CacheLineLocker* value) const
				{
					return value->HashNext();
				}
			};

			typedef BOpenHashTable<LockerHashDefinition> LockerTable;

			struct PagesDataOutput;

private:
			status_t			_ReadCacheLine(off_t lineOffset,
									size_t lineSize, off_t requestOffset,
							 		size_t requestLength, BDataIO* output);

			void				_DiscardPages(vm_page** pages, size_t firstPage,
									size_t pageCount);
			void				_CachePages(vm_page** pages, size_t firstPage,
									size_t pageCount);
			status_t			_WritePages(vm_page** pages,
									size_t pagesRelativeOffset,
									size_t requestLength, BDataIO* output);
			status_t			_ReadIntoPages(vm_page** pages,
									size_t firstPage, size_t pageCount);

			void				_LockCacheLine(CacheLineLocker* lineLocker);
			void				_UnlockCacheLine(CacheLineLocker* lineLocker);

private:
			static const size_t kCacheLineSize = 64 * 1024;
			static const size_t kPagesPerCacheLine
				= kCacheLineSize / B_PAGE_SIZE;

private:
			mutex				fLock;
			BAbstractBufferedDataReader* fReader;
			VMCache*			fCache;
			LockerTable			fCacheLineLockers;
};


#endif	// HEAP_CACHE_H
