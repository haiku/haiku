#ifndef _STDINT_H_
#define _STDINT_H_
/* 
** Distributed under the terms of the OpenBeOS License.
*/

typedef unsigned char uint8_t;
typedef signed char int8_t;

typedef unsigned short uint16_t;
typedef signed short int16_t;

typedef unsigned long uint32_t;
typedef signed long int32_t;

typedef unsigned long long uint64_t;
typedef signed long long int64_t;

typedef unsigned long long uintmax_t;
typedef signed long long intmax_t;

// least types

typedef uint8_t uint_least8_t;
typedef int8_t int_least8_t;

typedef uint16_t uint_least16_t;
typedef int16_t int_least16_t;

typedef uint32_t uint_least32_t;
typedef int32_t int_least32_t;

typedef uint64_t uint_least64_t;
typedef int64_t int_least64_t;

// fast types assume a 32-bit processor

typedef uint32_t uint_fast8_t;
typedef int32_t int_fast8_t;

typedef uint32_t uint_fast16_t;
typedef int32_t int_fast16_t;

typedef uint32_t uint_fast32_t;
typedef int32_t int_fast32_t;

typedef uint64_t uint_fast64_t;
typedef int64_t int_fast64_t;

/* BSD compatibility */
typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;

#endif	/* _STDINT_H_ */
