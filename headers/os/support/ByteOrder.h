// Modified BeOS header. Just here to be able to compile and test BResources.
// To be replaced by the OpenBeOS version to be provided by the IK Team.

#ifndef _sk_byte_order_h_
#define _sk_byte_order_h_

#include <BeBuild.h>
#include <endian.h>
#include <SupportDefs.h>
#include <TypeConstants.h>	/* For convenience */

#ifdef __cplusplus
extern "C" {
#endif 

/*----------------------------------------------------------------------*/
/*------- Swap-direction constants, and swapping functions -------------*/

typedef enum {
	B_SWAP_HOST_TO_LENDIAN,
	B_SWAP_HOST_TO_BENDIAN,
	B_SWAP_LENDIAN_TO_HOST,
	B_SWAP_BENDIAN_TO_HOST,
	B_SWAP_ALWAYS
} swap_action;

extern status_t swap_data(type_code type, void *data, size_t length,
						  swap_action action);

extern bool is_type_swapped(type_code type);
 
 
/*-----------------------------------------------------------------------*/
/*----- Private implementations -----------------------------------------*/
extern double __swap_double(double arg);
extern float  __swap_float(float arg);
extern uint64 __swap_int64(uint64 uarg);
extern uint32 __swap_int32(uint32 uarg);
extern uint16 __swap_int16(uint16 uarg);
/*-------------------------------------------------------------*/


/*-------------------------------------------------------------*/
/*--------- Host is Little  --------------------------------------*/

#if BYTE_ORDER == __LITTLE_ENDIAN
#define B_HOST_IS_LENDIAN 1
#define B_HOST_IS_BENDIAN 0

/*--------- Host Native -> Little  --------------------*/

#define B_HOST_TO_LENDIAN_DOUBLE(arg)	(double)(arg)
#define B_HOST_TO_LENDIAN_FLOAT(arg)	(float)(arg)
#define B_HOST_TO_LENDIAN_INT64(arg)	(uint64)(arg)
#define B_HOST_TO_LENDIAN_INT32(arg)	(uint32)(arg)
#define B_HOST_TO_LENDIAN_INT16(arg)	(uint16)(arg)

/*--------- Host Native -> Big  ------------------------*/
#define B_HOST_TO_BENDIAN_DOUBLE(arg)	__swap_double(arg)
#define B_HOST_TO_BENDIAN_FLOAT(arg)	__swap_float(arg)
#define B_HOST_TO_BENDIAN_INT64(arg)	__swap_int64(arg)
#define B_HOST_TO_BENDIAN_INT32(arg)	__swap_int32(arg)
#define B_HOST_TO_BENDIAN_INT16(arg)	__swap_int16(arg)

/*--------- Little -> Host Native ---------------------*/
#define B_LENDIAN_TO_HOST_DOUBLE(arg)	(double)(arg)
#define B_LENDIAN_TO_HOST_FLOAT(arg)	(float)(arg)
#define B_LENDIAN_TO_HOST_INT64(arg)	(uint64)(arg)
#define B_LENDIAN_TO_HOST_INT32(arg)	(uint32)(arg)
#define B_LENDIAN_TO_HOST_INT16(arg)	(uint16)(arg)

/*--------- Big -> Host Native ------------------------*/
#define B_BENDIAN_TO_HOST_DOUBLE(arg)	__swap_double(arg)
#define B_BENDIAN_TO_HOST_FLOAT(arg)	__swap_float(arg)
#define B_BENDIAN_TO_HOST_INT64(arg)	__swap_int64(arg)
#define B_BENDIAN_TO_HOST_INT32(arg)	__swap_int32(arg)
#define B_BENDIAN_TO_HOST_INT16(arg)	__swap_int16(arg)

#else /* __LITTLE_ENDIAN */


/*-------------------------------------------------------------*/
/*--------- Host is Big  --------------------------------------*/

#define B_HOST_IS_LENDIAN 0
#define B_HOST_IS_BENDIAN 1

/*--------- Host Native -> Little  -------------------*/
#define B_HOST_TO_LENDIAN_DOUBLE(arg)	__swap_double(arg)
#define B_HOST_TO_LENDIAN_FLOAT(arg)	__swap_float(arg)
#define B_HOST_TO_LENDIAN_INT64(arg)	__swap_int64(arg)
#define B_HOST_TO_LENDIAN_INT32(arg)	__swap_int32(arg)
#define B_HOST_TO_LENDIAN_INT16(arg)	__swap_int16(arg)

/*--------- Host Native -> Big  ------------------------*/
#define B_HOST_TO_BENDIAN_DOUBLE(arg)	(double)(arg)
#define B_HOST_TO_BENDIAN_FLOAT(arg)	(float)(arg)
#define B_HOST_TO_BENDIAN_INT64(arg)	(uint64)(arg)
#define B_HOST_TO_BENDIAN_INT32(arg)	(uint32)(arg)
#define B_HOST_TO_BENDIAN_INT16(arg)	(uint16)(arg)

/*--------- Little -> Host Native ----------------------*/
#define B_LENDIAN_TO_HOST_DOUBLE(arg)	__swap_double(arg)
#define B_LENDIAN_TO_HOST_FLOAT(arg)	__swap_float(arg)
#define B_LENDIAN_TO_HOST_INT64(arg)	__swap_int64(arg)
#define B_LENDIAN_TO_HOST_INT32(arg)	__swap_int32(arg)
#define B_LENDIAN_TO_HOST_INT16(arg)	__swap_int16(arg)

/*--------- Big -> Host Native -------------------------*/
#define B_BENDIAN_TO_HOST_DOUBLE(arg)	(double)(arg)
#define B_BENDIAN_TO_HOST_FLOAT(arg)	(float)(arg)
#define B_BENDIAN_TO_HOST_INT64(arg)	(uint64)(arg)
#define B_BENDIAN_TO_HOST_INT32(arg)	(uint32)(arg)
#define B_BENDIAN_TO_HOST_INT16(arg)	(uint16)(arg)

#endif /* __LITTLE_ENDIAN */


/*-------------------------------------------------------------*/
/*--------- Just-do-it macros ---------------------------------*/
#define B_SWAP_DOUBLE(arg)   __swap_double(arg)
#define B_SWAP_FLOAT(arg)    __swap_float(arg)
#define B_SWAP_INT64(arg)    __swap_int64(arg)
#define B_SWAP_INT32(arg)    __swap_int32(arg)
#define B_SWAP_INT16(arg)    __swap_int16(arg)


/*-------------------------------------------------------------*/
/*---------Berkeley macros -----------------------------------*/
#define htonl(x) B_HOST_TO_BENDIAN_INT32(x)
#define ntohl(x) B_BENDIAN_TO_HOST_INT32(x)
#define htons(x) B_HOST_TO_BENDIAN_INT16(x)
#define ntohs(x) B_BENDIAN_TO_HOST_INT16(x)

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif	// _sk_byte_order_h_
