#include <ByteOrder.h>
#include <OS.h>
#include "AudioConversion.h"
#include "RawFormats.h"

// XXX GCC doesn't always generate nice code...

typedef float float32;
typedef double float64;

class uint8_sample
{
public:
	inline operator uint8() { return data; }
	inline operator int8()  { return (int32)data - 128; }
	inline operator int16() { return ((int32)data - 128) << 8; }
	inline operator int32() { return ((int32)data - 128) << 24; }
	inline operator float() { return ((int32)data - 128) * (1.0f / 127.0f); }
private:
	uint8 data;
} _PACKED;

class int8_sample
{
public:
	inline operator uint8() { return (int32)data + 128; }
	inline operator int8()  { return data; }
	inline operator int16() { return (int16)data << 8; }
	inline operator int32() { return (int32)data << 24; }
	inline operator float() { return (int32)data * (1.0f / 127.0f); }
private:
	int8 data;
} _PACKED;

class int16_sample
{
public:
	inline operator uint8() { return (uint8)((int8)(data >> 8) + 128); }
	inline operator int8()  { return (int8)(data >> 8); }
	inline operator int16() { return data; }
	inline operator int32() { return (int32)data << 16; }
	inline operator float() { return data * (1.0f / 32767.0f); }
private:
	int16 data;
} _PACKED;

class int24_sample
{
public:
#if B_HOST_IS_LENDIAN
	inline operator uint8() { return (int32)data[2] + 128; }
	inline operator int8()  { return (int8)data[2]; }
	inline operator int16() { return (int16)((uint32)data[2] << 8 | (uint32)data[1]); }
	inline operator int32() { return (int32)((uint32)data[2] << 24 | (uint32)data[1] << 16 | (uint32)data[0] << 8); }
	inline operator float() { return (int32)((uint32)data[2] << 16 | (uint32)data[1] << 8 | (uint32)data[0]) * (1.0f / (2147483647.0f / 256)); }
// XXX is the line above correct? long version:
//	inline operator float() { return (int32)((uint32)data[2] << 24 | (uint32)data[1] << 16 | (uint32)data[0] << 8) * (1.0f / 2147483647.0f); }
#else
	inline operator uint8() { return (int32)data[0] + 128; }
	inline operator int8()  { return (int8)data[0]; }
	inline operator int16() { return (int16)((uint32)data[0] << 8 | (uint32)data[1]); }
	inline operator int32() { return (int32)((uint32)data[0] << 24 | (uint32)data[1] << 16 | (uint32)data[2] << 8); }
	inline operator float() { return (int32)((uint32)data[0] << 16 | (uint32)data[1] << 8 | (uint32)data[2]) * (1.0f / (2147483647.0f / 256)); }
// XXX is the line above correct? long version:
//	inline operator float() { return (int32)((uint32)data[0] << 24 | (uint32)data[1] << 16 | (uint32)data[2] << 8) * (1.0f / 2147483647.0f); }
#endif
private:
	uint8 data[3];
} _PACKED;

class int32_sample
{
public:
	inline operator uint8() { return (int8)(data >> 24) + 128; }
	inline operator int8()  { return (int8)(data >> 24); }
	inline operator int16() { return (int16)(data >> 16); }
	inline operator int32() { return data; }
	inline operator float() { return data * (1.0f / 2147483647.0f); }
private:
	int32 data;
} _PACKED;

class float32_sample
{
public:
	inline operator uint8() { int32 v = (int32)(data * 127.0f) + 128; if (v > 255) v = 255; else if (v < 0) v = 0; return v; }
	inline operator int8()  { int32 v = (int32)(data * 127.0f); if (v > 127) v = 127; else if (v < -127) v = -127; return v; }
	inline operator int16() { int32 v = (int32)(data * 32767.0f); if (v > 32767) v = 32767; else if (v < -32767) v = -32767; return v; }
	inline operator int32() { float32 v; if (data < -1.0f) v = -1.0f; else if (data > 1.0f) v = 1.0f; else v = data; return (int32)(v * 2147483647.0f); }
	inline operator float() { return data; }
private:
	float32 data;
} _PACKED;

class float64_sample
{
public:
	inline operator uint8() { int32 v = (int32)(data * 127.0f) + 128; if (v > 255) v = 255; else if (v < 0) v = 0; return v; }
	inline operator int8()  { int32 v = (int32)(data * 127.0f); if (v > 127) v = 127; else if (v < -127) v = -127; return v; }
	inline operator int16() { int32 v = (int32)(data * 32767.0f); if (v > 32767) v = 32767; else if (v < -32767) v = -32767; return v; }
	inline operator int32() { float64 v; if (data < -1.0) v = -1.0; else if (data > 1.0) v = 1.0; else v = data; return (int32)(v * 2147483647.0f); }
	inline operator float() { return data; }
private:
	float64 data;
} _PACKED;

#define CONVERT(src_type, dst_type)				\
void src_type##_to_##dst_type (void *dst, void *src, int32 count) \
{												\
	register src_type##_sample *s = (src_type##_sample *) src;	\
	register dst_type *d = (dst_type *) dst;	\
	register int32 c = count >> 4;				\
	if (!c) goto fin1;							\
	do {										\
		d[0] = s[0]; d[1] = s[1];				\
		d[2] = s[2]; d[3] = s[3];				\
		d[4] = s[4]; d[5] = s[5];				\
		d[6] = s[6]; d[7] = s[7];				\
		d[8] = s[8]; d[9] = s[9];				\
		d[10] = s[10]; d[11] = s[11];			\
		d[12] = s[12]; d[13] = s[13];			\
		d[14] = s[14]; d[15] = s[15];			\
		s += 16; d += 16;						\
	} while (--c);								\
fin1:											\
	c = count & 15;								\
	if (!c) goto fin2;							\
	do {										\
		*(d++) = *(s++);						\
	} while (--c);								\
fin2:											\
	;											\
}


CONVERT(uint8, uint8)
CONVERT(uint8, int8)
CONVERT(uint8, int16)
CONVERT(uint8, int32)
CONVERT(uint8, float32)

CONVERT(int8, uint8)
CONVERT(int8, int8)
CONVERT(int8, int16)
CONVERT(int8, int32)
CONVERT(int8, float32)

CONVERT(int16, uint8)
CONVERT(int16, int8)
CONVERT(int16, int16)
CONVERT(int16, int32)
CONVERT(int16, float32)

CONVERT(int24, uint8)
CONVERT(int24, int8)
CONVERT(int24, int16)
CONVERT(int24, int32)
CONVERT(int24, float32)

CONVERT(int32, uint8)
CONVERT(int32, int8)
CONVERT(int32, int16)
CONVERT(int32, int32)
CONVERT(int32, float32)

CONVERT(float32, uint8)
CONVERT(float32, int8)
CONVERT(float32, int16)
CONVERT(float32, int32)
CONVERT(float32, float32)

CONVERT(float64, uint8)
CONVERT(float64, int8)
CONVERT(float64, int16)
CONVERT(float64, int32)
CONVERT(float64, float32)

void
swap_int16(void *data, int32 count)
{
	swap_data(B_INT16_TYPE, data, count * 2, B_SWAP_ALWAYS);
}

void
swap_int24(void *data, int32 count)
{
	register int32 c = count;
	register uint8 *d = (uint8 *)data;
	if (!c)
		return;
	do {
		register uint8 temp = d[0];
		d[0] = d[2];
		d[2] = temp;
		d += 3;
	} while (--c);
}

void
swap_int32(void *data, int32 count)
{
	swap_data(B_INT32_TYPE, data, count * 4, B_SWAP_ALWAYS);
}

void
swap_float32(void *data, int32 count)
{
	swap_data(B_FLOAT_TYPE, data, count * 4, B_SWAP_ALWAYS);
}

void
swap_float64(void *data, int32 count)
{
	swap_data(B_INT64_TYPE, data, count * 8, B_SWAP_ALWAYS);
}
