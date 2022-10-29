/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_EXCLUSIVE_BORROW_H
#define _B_EXCLUSIVE_BORROW_H

#include <atomic>
#include <memory>

#include <ErrorsExt.h>

namespace BPrivate {

namespace Network {


class BBorrowError : public BError
{
public:
	BBorrowError(const char* origin)
		:
		BError(origin)
	{
	}

	virtual const char* Message() const noexcept override { return "BBorrowError"; }
};


class BorrowAdmin
{
private:
	static constexpr uint8 kOwned = 0x1;
	static constexpr uint8 kBorrowed = 0x2;
	std::atomic<uint8> fState = kOwned;

protected:
	virtual ~BorrowAdmin() = default;

	virtual void Cleanup() noexcept {};
	virtual void ReleasePointer() noexcept {};

public:
	BorrowAdmin() noexcept {}


	void Borrow()
	{
		auto alreadyBorrowed = (fState.fetch_or(kBorrowed) & kBorrowed) == kBorrowed;
		if (alreadyBorrowed) {
			throw BBorrowError(__PRETTY_FUNCTION__);
		}
	}


	void Return() noexcept
	{
		auto cleanup = (fState.fetch_and(~kBorrowed) & kOwned) != kOwned;
		if (cleanup)
			this->Cleanup();
	}


	void Forfeit() noexcept
	{
		auto cleanup = (fState.fetch_and(~kOwned) & kBorrowed) != kBorrowed;
		if (cleanup)
			this->Cleanup();
	}


	bool IsBorrowed() noexcept { return (fState.load() & kBorrowed) == kBorrowed; }


	void Release()
	{
		if ((fState.load() & kBorrowed) == kBorrowed)
			throw BBorrowError(__PRETTY_FUNCTION__);
		this->ReleasePointer();
		this->Cleanup();
	}
};


template<typename T> class BorrowPointer : public BorrowAdmin
{
public:
	BorrowPointer(T* object) noexcept
		:
		fPtr(object)
	{
	}

	virtual ~BorrowPointer() { delete fPtr; }

protected:
	virtual void Cleanup() noexcept override { delete this; }

	virtual void ReleasePointer() noexcept override { fPtr = nullptr; }

private:
	T* fPtr;
};


template<typename T> class BExclusiveBorrow
{
	template<typename P> friend class BBorrow;

	T* fPtr = nullptr;
	BorrowAdmin* fAdminBlock = nullptr;

public:
	BExclusiveBorrow() noexcept {}


	BExclusiveBorrow(nullptr_t) noexcept {}


	BExclusiveBorrow(T* object)
	{
		fAdminBlock = new BorrowPointer<T>(object);
		fPtr = object;
	}

	~BExclusiveBorrow()
	{
		if (fAdminBlock)
			fAdminBlock->Forfeit();
	}

	BExclusiveBorrow(const BExclusiveBorrow&) = delete;


	BExclusiveBorrow& operator=(const BExclusiveBorrow&) = delete;


	BExclusiveBorrow(BExclusiveBorrow&& other) noexcept
	{
		if (fAdminBlock)
			fAdminBlock->Forfeit();
		fAdminBlock = other.fAdminBlock;
		fPtr = other.fPtr;
		other.fAdminBlock = nullptr;
		other.fPtr = nullptr;
	}


	BExclusiveBorrow& operator=(BExclusiveBorrow&& other) noexcept
	{
		if (fAdminBlock)
			fAdminBlock->Forfeit();
		fAdminBlock = other.fAdminBlock;
		fPtr = other.fPtr;
		other.fAdminBlock = nullptr;
		other.fPtr = nullptr;
		return *this;
	}


	bool HasValue() const noexcept { return bool(fPtr); }


	T& operator*() const
	{
		if (fAdminBlock && !fAdminBlock->IsBorrowed())
			return *fPtr;
		throw BBorrowError(__PRETTY_FUNCTION__);
	}


	T* operator->() const
	{
		if (fAdminBlock && !fAdminBlock->IsBorrowed())
			return fPtr;
		throw BBorrowError(__PRETTY_FUNCTION__);
	}


	std::unique_ptr<T> Release()
	{
		if (!fAdminBlock)
			throw BBorrowError(__PRETTY_FUNCTION__);
		fAdminBlock->Release();
		auto retval = std::unique_ptr<T>(fPtr);
		fPtr = nullptr;
		fAdminBlock = nullptr;
		return retval;
	}
};


template<typename T> class BBorrow
{
	T* fPtr = nullptr;
	BorrowAdmin* fAdminBlock = nullptr;

public:
	BBorrow() noexcept {}


	BBorrow(nullptr_t) noexcept {}


	template<typename P>
	explicit BBorrow(BExclusiveBorrow<P>& owner)
		:
		fPtr(owner.fPtr),
		fAdminBlock(owner.fAdminBlock)
	{
		fAdminBlock->Borrow();
	}


	BBorrow(const BBorrow&) = delete;


	BBorrow& operator=(const BBorrow&) = delete;


	BBorrow(BBorrow&& other) noexcept
		:
		fPtr(other.fPtr),
		fAdminBlock(other.fAdminBlock)
	{
		other.fPtr = nullptr;
		other.fAdminBlock = nullptr;
	}


	BBorrow& operator=(BBorrow&& other) noexcept
	{
		if (fAdminBlock)
			fAdminBlock->Return();

		fPtr = other.fPtr;
		fAdminBlock = other.fAdminBlock;
		other.fPtr = nullptr;
		other.fAdminBlock = nullptr;
		return *this;
	}


	~BBorrow()
	{
		if (fAdminBlock)
			fAdminBlock->Return();
	}


	bool HasValue() const noexcept { return bool(fPtr); }


	T& operator*() const
	{
		if (fPtr)
			return *fPtr;
		throw BBorrowError(__PRETTY_FUNCTION__);
	}


	T* operator->() const
	{
		if (fPtr)
			return fPtr;
		throw BBorrowError(__PRETTY_FUNCTION__);
	}


	void Return() noexcept
	{
		if (fAdminBlock)
			fAdminBlock->Return();
		fAdminBlock = nullptr;
		fPtr = nullptr;
	}
};


template<class T, class... _Args>
BExclusiveBorrow<T>
make_exclusive_borrow(_Args&&... __args)
{
	auto guardedObject = std::make_unique<T>(std::forward<_Args>(__args)...);
	auto retval = BExclusiveBorrow<T>(guardedObject.get());
	guardedObject.release();
	return retval;
}


} // namespace Network

} // namespace BPrivate

#endif // _B_EXCLUSIVE_BORROW_H
