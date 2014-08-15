/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 */

#include <atomic>
#include <sys/types.h>

#ifndef ATOMIC_FUNCS_ARE_SYSCALLS

extern "C" int32_t
atomic_get_and_set(int32_t* ptr, int32_t value)
{
    auto& obj = *reinterpret_cast<std::atomic<int32_t>*>(ptr);
    return obj.exchange(value);
}

#endif /* ATOMIC64_FUNCS_ARE_SYSCALLS */
