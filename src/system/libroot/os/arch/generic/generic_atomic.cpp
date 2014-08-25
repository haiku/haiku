/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 */

#include <atomic>
#include <sys/types.h>


extern "C" [[gnu::optimize("omit-frame-pointer")]] void
atomic_set(int32_t* ptr, int32_t value)
{
	auto& obj = *reinterpret_cast<std::atomic<int32_t>*>(ptr);
	obj.store(value, std::memory_order_release);
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int32_t
atomic_get_and_set(int32_t* ptr, int32_t value)
{
	auto& obj = *reinterpret_cast<std::atomic<int32_t>*>(ptr);
	return obj.exchange(value);
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int32_t
atomic_test_and_set(int32_t* ptr, int32_t desired, int32_t expected)
{
	auto& obj = *reinterpret_cast<std::atomic<int32_t>*>(ptr);
	obj.compare_exchange_strong(expected, desired);
	return expected;
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int32_t
atomic_add(int32_t* ptr, int32_t value)
{
	auto& obj = *reinterpret_cast<std::atomic<int32_t>*>(ptr);
	return obj.fetch_add(value);
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int32_t
atomic_and(int32_t* ptr, int32_t value)
{
	auto& obj = *reinterpret_cast<std::atomic<int32_t>*>(ptr);
	return obj.fetch_and(value);
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int32_t
atomic_or(int32_t* ptr, int32_t value)
{
	auto& obj = *reinterpret_cast<std::atomic<int32_t>*>(ptr);
	return obj.fetch_or(value);
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int32_t
atomic_get(int32_t* ptr)
{
	auto& obj = *reinterpret_cast<std::atomic<int32_t>*>(ptr);
	return obj.load(std::memory_order_acquire);
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] void
atomic_set64(int64_t* ptr, int64_t value)
{
	auto& obj = *reinterpret_cast<std::atomic<int64_t>*>(ptr);
	obj.store(value, std::memory_order_release);
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int64_t
atomic_get_and_set64(int64_t* ptr, int64_t value)
{
	auto& obj = *reinterpret_cast<std::atomic<int64_t>*>(ptr);
	return obj.exchange(value);
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int64_t
atomic_test_and_set64(int64_t* ptr, int64_t desired, int64_t expected)
{
	auto& obj = *reinterpret_cast<std::atomic<int64_t>*>(ptr);
	obj.compare_exchange_strong(expected, desired);
	return expected;
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int64_t
atomic_add64(int64_t* ptr, int64_t value)
{
	auto& obj = *reinterpret_cast<std::atomic<int64_t>*>(ptr);
	return obj.fetch_add(value);
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int64_t
atomic_and64(int64_t* ptr, int64_t value)
{
	auto& obj = *reinterpret_cast<std::atomic<int64_t>*>(ptr);
	return obj.fetch_and(value);
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int64_t
atomic_or64(int64_t* ptr, int64_t value)
{
	auto& obj = *reinterpret_cast<std::atomic<int64_t>*>(ptr);
	return obj.fetch_or(value);
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int64_t
atomic_get64(int64_t* ptr)
{
	auto& obj = *reinterpret_cast<std::atomic<int64_t>*>(ptr);
	return obj.load(std::memory_order_acquire);
}

