/*
 * Copyright 2015, Haiku, Inc. All RightsReserved.
 * Copyright 2002, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef LENDIAN_BITFIELD_H
#define LENDIAN_BITFIELD_H


#include <ByteOrder.h>


#if B_HOST_IS_BENDIAN

#define B_LBITFIELD8_2(b1,b2)						uint8 b2,b1
#define B_LBITFIELD8_3(b1,b2,b3)					uint8 b3,b2,b1
#define B_LBITFIELD8_4(b1,b2,b3,b4)					uint8 b4,b3,b2,b1
#define B_LBITFIELD8_5(b1,b2,b3,b4,b5)				uint8 b5,b4,b3,b2,b1
#define B_LBITFIELD8_6(b1,b2,b3,b4,b5,b6)			uint8 b6,b5,b4,b3,b2,b1
#define B_LBITFIELD8_7(b1,b2,b3,b4,b5,b6,b7)		uint8 b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD8_8(b1,b2,b3,b4,b5,b6,b7,b8)		uint8 b8,b7,b6,b5,b4,b3,b2,b1


#define B_LBITFIELD16_2(b1,b2)														uint16 b2,b1
#define B_LBITFIELD16_3(b1,b2,b3)													uint16 b3,b2,b1
#define B_LBITFIELD16_4(b1,b2,b3,b4)												uint16 b4,b3,b2,b1
#define B_LBITFIELD16_5(b1,b2,b3,b4,b5)												uint16 b5,b4,b3,b2,b1
#define B_LBITFIELD16_6(b1,b2,b3,b4,b5,b6)											uint16 b6,b5,b4,b3,b2,b1
#define B_LBITFIELD16_7(b1,b2,b3,b4,b5,b6,b7)								 		uint16 b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD16_8(b1,b2,b3,b4,b5,b6,b7,b8)									uint16 b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD16_9(b1,b2,b3,b4,b5,b6,b7,b8,b9)									uint16 b9,b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD16_10(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10)							uint16 b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD16_11(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11)						uint16 b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD16_12(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12)					uint16 b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD16_13(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13)				uint16 b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD16_14(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14)			uint16 b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD16_15(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15)		uint16 b15,b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD16_16(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16)	uint16 b16,b15,b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1

#define B_LBITFIELD32_2(b1,b2)														uint32 b2,b1
#define B_LBITFIELD32_3(b1,b2,b3)													uint32 b3,b2,b1
#define B_LBITFIELD32_4(b1,b2,b3,b4)												uint32 b4,b3,b2,b1
#define B_LBITFIELD32_5(b1,b2,b3,b4,b5)												uint32 b5,b4,b3,b2,b1
#define B_LBITFIELD32_6(b1,b2,b3,b4,b5,b6)											uint32 b6,b5,b4,b3,b2,b1
#define B_LBITFIELD32_7(b1,b2,b3,b4,b5,b6,b7)							 			uint32 b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD32_8(b1,b2,b3,b4,b5,b6,b7,b8)									uint32 b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD32_9(b1,b2,b3,b4,b5,b6,b7,b8,b9)									uint32 b9,b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD32_10(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10)							uint32 b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD32_11(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11)						uint32 b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD32_12(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12)					uint32 b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD32_13(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13)				uint32 b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD32_14(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14)			uint32 b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD32_15(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15)		uint32 b15,b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1
#define B_LBITFIELD32_16(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16)	uint32 b16,b15,b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1

#elif B_HOST_IS_LENDIAN

#define B_LBITFIELD8_2(b1,b2)						uint8 b1,b2
#define B_LBITFIELD8_3(b1,b2,b3)					uint8 b1,b2,b3
#define B_LBITFIELD8_4(b1,b2,b3,b4)					uint8 b1,b2,b3,b4
#define B_LBITFIELD8_5(b1,b2,b3,b4,b5)				uint8 b1,b2,b3,b4,b5
#define B_LBITFIELD8_6(b1,b2,b3,b4,b5,b6)			uint8 b1,b2,b3,b4,b5,b6
#define B_LBITFIELD8_7(b1,b2,b3,b4,b5,b6,b7)		uint8 b1,b2,b3,b4,b5,b6,b7
#define B_LBITFIELD8_8(b1,b2,b3,b4,b5,b6,b7,b8)		uint8 b1,b2,b3,b4,b5,b6,b7,b8

#define B_LBITFIELD16_2(b1,b2)														uint16 b1,b2
#define B_LBITFIELD16_3(b1,b2,b3)													uint16 b1,b2,b3
#define B_LBITFIELD16_4(b1,b2,b3,b4)												uint16 b1,b2,b3,b4
#define B_LBITFIELD16_5(b1,b2,b3,b4,b5)												uint16 b1,b2,b3,b4,b5
#define B_LBITFIELD16_6(b1,b2,b3,b4,b5,b6)											uint16 b1,b2,b3,b4,b5,b6
#define B_LBITFIELD16_7(b1,b2,b3,b4,b5,b6,b7)										uint16 b1,b2,b3,b4,b5,b6,b7
#define B_LBITFIELD16_8(b1,b2,b3,b4,b5,b6,b7,b8)									uint16 b1,b2,b3,b4,b5,b6,b7,b8
#define B_LBITFIELD16_9(b1,b2,b3,b4,b5,b6,b7,b8,b9) 								uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9
#define B_LBITFIELD16_10(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10)							uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10
#define B_LBITFIELD16_11(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11)						uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11
#define B_LBITFIELD16_12(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12)					uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12
#define B_LBITFIELD16_13(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13)				uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13
#define B_LBITFIELD16_14(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14)			uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14
#define B_LBITFIELD16_15(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15)		uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15
#define B_LBITFIELD16_16(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16)	uint16 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16

#define B_LBITFIELD32_2(b1,b2)														uint32 b1,b2
#define B_LBITFIELD32_3(b1,b2,b3)													uint32 b1,b2,b3
#define B_LBITFIELD32_4(b1,b2,b3,b4)												uint32 b1,b2,b3,b4
#define B_LBITFIELD32_5(b1,b2,b3,b4,b5)												uint32 b1,b2,b3,b4,b5
#define B_LBITFIELD32_6(b1,b2,b3,b4,b5,b6)											uint32 b1,b2,b3,b4,b5,b6
#define B_LBITFIELD32_7(b1,b2,b3,b4,b5,b6,b7)										uint32 b1,b2,b3,b4,b5,b6,b7
#define B_LBITFIELD32_8(b1,b2,b3,b4,b5,b6,b7,b8)									uint32 b1,b2,b3,b4,b5,b6,b7,b8
#define B_LBITFIELD32_9(b1,b2,b3,b4,b5,b6,b7,b8,b9) 								uint32 b1,b2,b3,b4,b5,b6,b7,b8,b9
#define B_LBITFIELD32_10(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10)							uint32 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10
#define B_LBITFIELD32_11(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11)						uint32 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11
#define B_LBITFIELD32_12(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12)					uint32 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12
#define B_LBITFIELD32_13(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13)				uint32 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13
#define B_LBITFIELD32_14(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14)			uint32 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14
#define B_LBITFIELD32_15(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15)		uint32 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15
#define B_LBITFIELD32_16(b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16)	uint32 b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16

#else
#	error "Unknown host endianness"
#endif

#endif /* LENDIAN_BITFIELD_H */
