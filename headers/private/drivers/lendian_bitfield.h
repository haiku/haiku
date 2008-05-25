/*
** Copyright 2002, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#ifndef LENDIAN_BITFIELD_H
#define LENDIAN_BITFIELD_H

#include <ByteOrder.h>

#if B_HOST_IS_BENDIAN

#define  LBITFIELD8_2(b1,b2)					uint8 b2,b1
#define  LBITFIELD8_3(b1,b2,b3)					uint8 b3,b2,b1
#define  LBITFIELD8_4(b1,b2,b3,b4)				uint8 b4,b3,b2,b1
#define  LBITFIELD8_5(b1,b2,b3,b4,b5)			uint8 b5,b4,b3,b2,b1
#define  LBITFIELD8_6(b1,b2,b3,b4,b5,b6)		uint8 b6,b5,b4,b3,b2,b1
#define  LBITFIELD8_7(b1,b2,b3,b4,b5,b6,b7)		uint8 b7,b6,b5,b4,b3,b2,b1
#define  LBITFIELD8_8(b1,b2,b3,b4,b5,b6,b7,b8)	uint8 b8,b7,b6,b5,b4,b3,b2,b1


#define  LBITFIELD2(b1,b2)													uint16 b2,b1
#define  LBITFIELD3(b1,b2,b3)												uint16 b3,b2,b1
#define  LBITFIELD4(b1,b2,b3,b4)											uint16 b4,b3,b2,b1
#define  LBITFIELD5(b1,b2,b3,b4,b5)											uint16 b5,b4,b3,b2,b1
#define  LBITFIELD6(b1,b2,b3,b4,b5,b6)										uint16 b6,b5,b4,b3,b2,b1
#define  LBITFIELD7(b1,b2,b3,b4,b5,b6,b7)							 		uint16 b7,b6,b5,b4,b3,b2,b1
#define  LBITFIELD8(b1,b2,b3,b4,b5,b6,b7,b8)								uint16 b8,b7,b6,b5,b4,b3,b2,b1
#define  LBITFIELD9(b1,b2,b3,b4,b5,b6,b7,b8,b9)								uint16 b9,b8,b7,b6,b5,b4,b3,b2,b1
#define LBITFIELD10(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10)							uint16 b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define LBITFIELD11(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11)						uint16 b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define LBITFIELD12(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12)					uint16 b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define LBITFIELD13(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13)				uint16 b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define LBITFIELD14(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14)			uint16 b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define LBITFIELD15(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15)		uint16 b15,b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define LBITFIELD16(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16)	uint16 b16,b15,b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#elif B_HOST_IS_LENDIAN

#define  LBITFIELD8_2(b1,b2)					uint8 b1,b2
#define  LBITFIELD8_3(b1,b2,b3)					uint8 b1,b2,b3
#define  LBITFIELD8_4(b1,b2,b3,b4)				uint8 b1,b2,b3,b4
#define  LBITFIELD8_5(b1,b2,b3,b4,b5)			uint8 b1,b2,b3,b4,b5
#define  LBITFIELD8_6(b1,b2,b3,b4,b5,b6)		uint8 b1,b2,b3,b4,b5,b6
#define  LBITFIELD8_7(b1,b2,b3,b4,b5,b6,b7)		uint8 b1,b2,b3,b4,b5,b6,b7
#define  LBITFIELD8_8(b1,b2,b3,b4,b5,b6,b7,b8)	uint8 b1,b2,b3,b4,b5,b6,b7,b8

#define  LBITFIELD2(b1,b2)													uint16 b1,b2
#define  LBITFIELD3(b1,b2,b3)												uint16 b1,b2,b3
#define  LBITFIELD4(b1,b2,b3,b4)											uint16 b1,b2,b3,b4
#define  LBITFIELD5(b1,b2,b3,b4,b5)											uint16 b1,b2,b3,b4,b5
#define  LBITFIELD6(b1,b2,b3,b4,b5,b6)										uint16 b1,b2,b3,b4,b5,b6
#define  LBITFIELD7(b1,b2,b3,b4,b5,b6,b7)									uint16 b1,b2,b3,b4,b5,b6,b7
#define  LBITFIELD8(b1,b2,b3,b4,b5,b6,b7,b8)								uint16 b1,b2,b3,b4,b5,b6,b7,b8
#define  LBITFIELD9(b1,b2,b3,b4,b5,b6,b7,b8,b9) 							uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9
#define LBITFIELD10(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10)							uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10
#define LBITFIELD11(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11)						uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11
#define LBITFIELD12(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12)					uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12
#define LBITFIELD13(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13)				uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13
#define LBITFIELD14(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14)			uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14
#define LBITFIELD15(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15)		uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15
#define LBITFIELD16(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16)	uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16

#else
#error "Unknown host endianness"
#endif

#endif /* LENDIAN_BITFIELD_H */
