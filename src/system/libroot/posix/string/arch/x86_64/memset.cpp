/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */


#include <array>

#include <cstddef>
#include <cstdint>

#include <emmintrin.h>


static inline void
memset_repstos(uint8_t* destination, uint8_t value, size_t length)
{
	__asm__ __volatile__("rep stosb"
		: "+D" (destination), "+c" (length)
		: "a" (value)
		: "memory");
}


static inline void
memset_sse(uint8_t* destination, uint8_t value, size_t length)
{
	__m128i packed = _mm_set1_epi8(value);
	auto end = reinterpret_cast<__m128i*>(destination + length - 16);
	auto diff = reinterpret_cast<uintptr_t>(destination) % 16;
	if (diff) {
		diff = 16 - diff;
		length -= diff;
		_mm_storeu_si128(reinterpret_cast<__m128i*>(destination), packed);
	}
	auto ptr = reinterpret_cast<__m128i*>(destination + diff);
	while (length >= 64) {
		_mm_store_si128(ptr++, packed);
		_mm_store_si128(ptr++, packed);
		_mm_store_si128(ptr++, packed);
		_mm_store_si128(ptr++, packed);
		length -= 64;
	}
	while (length >= 16) {
		_mm_store_si128(ptr++, packed);
		length -= 16;
	}
	_mm_storeu_si128(end, packed);
}


static inline void
memset_small(uint8_t* destination, uint8_t value, size_t length)
{
	if (length >= 8) {
		auto packed = value * 0x101010101010101ul;
		auto ptr = reinterpret_cast<uint64_t*>(destination);
		auto end = reinterpret_cast<uint64_t*>(destination + length - 8);
		while (length >= 8) {
			*ptr++ = packed;
			length -= 8;
		}
		*end = packed;
	} else {
		while (length--) {
			*destination++ = value;
		}
	}
}


extern "C" void*
memset(void* ptr, int chr, size_t length)
{
	auto value = static_cast<unsigned char>(chr);
	auto destination = static_cast<uint8_t*>(ptr);
	if (length < 32) {
		memset_small(destination, value, length);
		return ptr;
	}
	if (length < 2048) {
		memset_sse(destination, value, length);
		return ptr;
	}
	memset_repstos(destination, value, length);
	return ptr;
}

