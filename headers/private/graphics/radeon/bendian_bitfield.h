#ifndef BENDIAN_BITFIELD_H
#define BENDIAN_BITFIELD_H

#include <ByteOrder.h>

#if B_HOST_IS_LENDIAN

#define  BBITFIELD8_2(b1,b2)					uint8 b2,b1
#define  BBITFIELD8_3(b1,b2,b3)					uint8 b3,b2,b1
#define  BBITFIELD8_4(b1,b2,b3,b4)				uint8 b4,b3,b2,b1
#define  BBITFIELD8_5(b1,b2,b3,b4,b5)			uint8 b5,b4,b3,b2,b1
#define  BBITFIELD8_6(b1,b2,b3,b4,b5,b6)		uint8 b6,b5,b4,b3,b2,b1
#define  BBITFIELD8_7(b1,b2,b3,b4,b5,b6,b7)		uint8 b7,b6,b5,b4,b3,b2,b1
#define  BBITFIELD8_8(b1,b2,b3,b4,b5,b6,b7,b8)	uint8 b8,b7,b6,b5,b4,b3,b2,b1


#define  BBITFIELD16_2(b1,b2)													uint16 b2,b1
#define  BBITFIELD16_3(b1,b2,b3)												uint16 b3,b2,b1
#define  BBITFIELD16_4(b1,b2,b3,b4)												uint16 b4,b3,b2,b1
#define  BBITFIELD16_5(b1,b2,b3,b4,b5)											uint16 b5,b4,b3,b2,b1
#define  BBITFIELD16_6(b1,b2,b3,b4,b5,b6)										uint16 b6,b5,b4,b3,b2,b1
#define  BBITFIELD16_7(b1,b2,b3,b4,b5,b6,b7)							 		uint16 b7,b6,b5,b4,b3,b2,b1
#define  BBITFIELD16_8(b1,b2,b3,b4,b5,b6,b7,b8)									uint16 b8,b7,b6,b5,b4,b3,b2,b1
#define  BBITFIELD16_9(b1,b2,b3,b4,b5,b6,b7,b8,b9)								uint16 b9,b8,b7,b6,b5,b4,b3,b2,b1
#define BBITFIELD16_10(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10)							uint16 b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define BBITFIELD16_11(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11)						uint16 b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define BBITFIELD16_12(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12)					uint16 b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define BBITFIELD16_13(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13)				uint16 b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define BBITFIELD16_14(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14)			uint16 b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define BBITFIELD16_15(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15)		uint16 b15,b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define BBITFIELD16_16(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16)	uint16 b16,b15,b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#elif B_HOST_IS_BENDIAN

#define  BBITFIELD8_2(b1,b2)					uint8 b1,b2
#define  BBITFIELD8_3(b1,b2,b3)					uint8 b1,b2,b3
#define  BBITFIELD8_4(b1,b2,b3,b4)				uint8 b1,b2,b3,b4
#define  BBITFIELD8_5(b1,b2,b3,b4,b5)			uint8 b1,b2,b3,b4,b5
#define  BBITFIELD8_6(b1,b2,b3,b4,b5,b6)		uint8 b1,b2,b3,b4,b5,b6
#define  BBITFIELD8_7(b1,b2,b3,b4,b5,b6,b7)		uint8 b1,b2,b3,b4,b5,b6,b7
#define  BBITFIELD8_8(b1,b2,b3,b4,b5,b6,b7,b8)	uint8 b1,b2,b3,b4,b5,b6,b7,b8

#define  BBITFIELD16_2(b1,b2)													uint16 b1,b2
#define  BBITFIELD16_3(b1,b2,b3)												uint16 b1,b2,b3
#define  BBITFIELD16_4(b1,b2,b3,b4)												uint16 b1,b2,b3,b4
#define  BBITFIELD16_5(b1,b2,b3,b4,b5)											uint16 b1,b2,b3,b4,b5
#define  BBITFIELD16_6(b1,b2,b3,b4,b5,b6)										uint16 b1,b2,b3,b4,b5,b6
#define  BBITFIELD16_7(b1,b2,b3,b4,b5,b6,b7)									uint16 b1,b2,b3,b4,b5,b6,b7
#define  BBITFIELD16_8(b1,b2,b3,b4,b5,b6,b7,b8)									uint16 b1,b2,b3,b4,b5,b6,b7,b8
#define  BBITFIELD16_9(b1,b2,b3,b4,b5,b6,b7,b8,b9) 								uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9
#define BBITFIELD16_10(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10)							uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10
#define BBITFIELD16_11(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11)						uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11
#define BBITFIELD16_12(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12)					uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12
#define BBITFIELD16_13(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13)				uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13
#define BBITFIELD16_14(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14)			uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14
#define BBITFIELD16_15(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15)		uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15
#define BBITFIELD16_16(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16)	uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16

#else
#error "Unknown host endianness"
#endif

#endif /* BENDIAN_BITFIELD_H */
