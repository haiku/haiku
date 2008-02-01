/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */


#ifdef BT_DEBUG_THIS_MODULE
	#ifndef MODULE_NAME
	//#warning MODULE_NAME not defined for Haiku BT debugging tools
	#define MODULE_NAME "BT"
	#endif
	
	#ifndef SUBMODULE_NAME
	//#warning SUBMODULE_NAME not defined for Haiku BT debugging tools
	#define SUBMODULE_NAME ""
	#endif
	
	#ifndef SUBMODULE_COLOR
	//#warning SUBMODULE_COLOR not defined for Haiku BT debugging tools
	#define SUBMODULE_COLOR 38
	#endif

	#define debugf(a,param...) dprintf("\x1b[%dm" MODULE_NAME " " SUBMODULE_NAME " " "%s\x1b[0m: " a,SUBMODULE_COLOR,__FUNCTION__, param);
	#define flowf(a) dprintf("\x1b[%dm" MODULE_NAME " " SUBMODULE_NAME " " "%s\x1b[0m: " a,SUBMODULE_COLOR,__FUNCTION__);
#else
	#define debugf(a,param...)
	#define flowf(a,param...)
#endif
#undef BT_DEBUG_THIS_MODULE

#define TOUCH(x) ((void)(x))

/* */ 
#if 0
#pragma mark - Kernel Auxiliary Stuff -
#endif

/* tricking bits */
#define SET_BIT(byte,bit_mask)			{byte|=bit_mask;}
#define CLEAR_BIT(byte,bit_mask)		{byte&=~bit_mask;}
#define GET_BIT(byte,bit_mask)			((byte&bit_mask)!=0)
#define TOOGLE_BIT(byte,bit_mask)		{byte^=bit_mask;}

//#define TEST_AND_SET(byte,bit_mask)     (((byte|=bit_mask)&bit_mask)!=0) 
//#define TEST_AND_CLEAR(byte,bit_mask)   (((byte&=~bit_mask)&bit_mask)!=0) 

static inline uint32 TEST_AND_SET(uint32 *byte, uint32 bit_mask) {

	uint32 val = (*byte&bit_mask)!=0;
	*byte |= bit_mask;
	return val;
}

static inline uint32 TEST_AND_CLEAR(uint32* byte, uint32 bit_mask) {

	uint32 val = (*byte&bit_mask)!=0;
	*byte &= ~bit_mask;
	return val;
}
