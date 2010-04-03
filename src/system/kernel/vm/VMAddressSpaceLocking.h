/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VM_ADDRESS_SPACE_LOCKING_H
#define VM_ADDRESS_SPACE_LOCKING_H


#include <OS.h>

#include <vm/VMAddressSpace.h>


struct VMAddressSpace;
struct VMArea;
struct VMCache;


class AddressSpaceLockerBase {
public:
	static	VMAddressSpace*		GetAddressSpaceByAreaID(area_id id);
};


class AddressSpaceReadLocker : private AddressSpaceLockerBase {
public:
								AddressSpaceReadLocker(team_id team);
								AddressSpaceReadLocker(VMAddressSpace* space,
									bool getNewReference);
								AddressSpaceReadLocker();
								~AddressSpaceReadLocker();

			status_t			SetTo(team_id team);
			void				SetTo(VMAddressSpace* space,
									bool getNewReference);
			status_t			SetFromArea(area_id areaID, VMArea*& area);

			bool				IsLocked() const { return fLocked; }
			bool				Lock();
			void				Unlock();

			void				Unset();

			VMAddressSpace*		AddressSpace() const { return fSpace; }

private:
			VMAddressSpace*		fSpace;
			bool				fLocked;
};


class AddressSpaceWriteLocker : private AddressSpaceLockerBase {
public:
								AddressSpaceWriteLocker(team_id team);
								AddressSpaceWriteLocker(VMAddressSpace* space,
									bool getNewReference);
								AddressSpaceWriteLocker();
								~AddressSpaceWriteLocker();

			status_t			SetTo(team_id team);
			void				SetTo(VMAddressSpace* space,
									bool getNewReference);
			status_t			SetFromArea(area_id areaID, VMArea*& area);
			status_t			SetFromArea(team_id team, area_id areaID,
									bool allowKernel, VMArea*& area);
			status_t			SetFromArea(team_id team, area_id areaID,
									VMArea*& area);

			bool				IsLocked() const { return fLocked; }
			void				Unlock();

			void				DegradeToReadLock();
			void				Unset();

			VMAddressSpace*		AddressSpace() const { return fSpace; }

private:
			VMAddressSpace*		fSpace;
			bool				fLocked;
			bool				fDegraded;
};


class MultiAddressSpaceLocker : private AddressSpaceLockerBase {
public:
								MultiAddressSpaceLocker();
								~MultiAddressSpaceLocker();

	inline	status_t			AddTeam(team_id team, bool writeLock,
									VMAddressSpace** _space = NULL);
	inline	status_t			AddArea(area_id area, bool writeLock,
									VMAddressSpace** _space = NULL);

			status_t			AddAreaCacheAndLock(area_id areaID,
									bool writeLockThisOne, bool writeLockOthers,
									VMArea*& _area, VMCache** _cache = NULL);

			status_t			Lock();
			void				Unlock();
			bool				IsLocked() const { return fLocked; }

			void				Unset();

private:
			struct lock_item {
				VMAddressSpace*	space;
				bool			write_lock;
			};

			bool				_ResizeIfNeeded();
			int32				_IndexOfAddressSpace(VMAddressSpace* space)
									const;
			status_t			_AddAddressSpace(VMAddressSpace* space,
									bool writeLock, VMAddressSpace** _space);

	static	int					_CompareItems(const void* _a, const void* _b);

			lock_item*			fItems;
			int32				fCapacity;
			int32				fCount;
			bool				fLocked;
};


inline status_t
MultiAddressSpaceLocker::AddTeam(team_id team, bool writeLock,
	VMAddressSpace** _space)
{
	return _AddAddressSpace(VMAddressSpace::Get(team), writeLock, _space);
}


inline status_t
MultiAddressSpaceLocker::AddArea(area_id area, bool writeLock,
	VMAddressSpace** _space)
{
	return _AddAddressSpace(GetAddressSpaceByAreaID(area), writeLock, _space);
}


#endif	// VM_ADDRESS_SPACE_LOCKING_H
