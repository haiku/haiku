/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */


#include <array>

#include <cstddef>
#include <cstdint>

#include <emmintrin.h>


namespace {


// __m128i resolves to a type with an attribute, which can't get into the
// template signature, resulting in a warning. Nonetheless the code is what we
// expect, so we silent the warning.
#pragma GCC diagnostic push
#if defined __GNUC__ && __GNUC__ >= 6
#pragma GCC diagnostic ignored "-Wignored-attributes"
#endif


template<template<size_t N> class Generator, unsigned N, unsigned ...Index>
struct GenerateTable : GenerateTable<Generator, N - 1,  N - 1, Index...> {
};

template<template<size_t N> class Generator, unsigned ...Index>
struct GenerateTable<Generator, 0, Index...>
	: std::array<decltype(Generator<0>::sValue), sizeof...(Index)> {
	constexpr GenerateTable()
	:
	std::array<decltype(Generator<0>::sValue), sizeof...(Index)> {
		{ Generator<Index>::sValue... }
	}
	{
	}
};


#pragma GCC diagnostic pop


static inline void memcpy_repmovs(uint8_t* destination, const uint8_t* source,
	size_t length)
{
	__asm__ __volatile__("rep movsb"
		: "+D" (destination), "+S" (source), "+c" (length)
		:
		: "memory");
}


template<size_t N>
inline void copy_small(uint8_t* destination, const uint8_t* source)
{
	struct data {
		uint8_t x[N];
	};
	*reinterpret_cast<data*>(destination)
		= *reinterpret_cast<const data*>(source);
}


template<size_t N>
struct SmallGenerator {
	constexpr static void (*sValue)(uint8_t*, const uint8_t*) = copy_small<N>;
};
constexpr static GenerateTable<SmallGenerator, 8> table_small;


static inline void memcpy_small(uint8_t* destination, const uint8_t* source,
	size_t length)
{
	if (length < 8) {
		table_small[length](destination, source);
	} else {
		auto to = reinterpret_cast<uint64_t*>(destination);
		auto from = reinterpret_cast<const uint64_t*>(source);
		*to = *from;
		to = reinterpret_cast<uint64_t*>(destination + length - 8);
		from = reinterpret_cast<const uint64_t*>(source + length - 8);
		*to = *from;
	}
}


template<size_t N>
inline void copy_sse(__m128i* destination, const __m128i* source)
{
	auto temp = _mm_loadu_si128(source);
	_mm_storeu_si128(destination, temp);
	copy_sse<N - 1>(destination + 1, source + 1);
}


template<>
inline void copy_sse<0>(__m128i* destination, const __m128i* source)
{
}


template<size_t N>
struct SSEGenerator {
	constexpr static void (*sValue)(__m128i*, const __m128i*) = copy_sse<N>;
};
constexpr static GenerateTable<SSEGenerator, 4> table_sse;


static inline void memcpy_sse(uint8_t* destination, const uint8_t* source, size_t length)
{
	auto to = reinterpret_cast<__m128i*>(destination);
	auto from = reinterpret_cast<const __m128i*>(source);
	auto toEnd = reinterpret_cast<__m128i*>(destination + length - 16);
	auto fromEnd = reinterpret_cast<const __m128i*>(source + length - 16);
	while (length >= 64) {
		copy_sse<4>(to, from);
		to += 4;
		from += 4;
		length -= 64;
	}
	if (length >= 16) {
		table_sse[length / 16](to, from);
		length %= 16;
	}
	if (length) {
		copy_sse<1>(toEnd, fromEnd);
	}
}


}


extern "C" void* memcpy(void* destination, const void* source, size_t length)
{
	auto to = static_cast<uint8_t*>(destination);
	auto from = static_cast<const uint8_t*>(source);
	if (length <= 16) {
		memcpy_small(to, from, length);
		return destination;
	}
	if (length < 2048) {
		memcpy_sse(to, from, length);
		return destination;
	}
	memcpy_repmovs(to, from, length);
	return destination;
}


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

