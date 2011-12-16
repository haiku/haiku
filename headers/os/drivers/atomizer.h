/* 
 * Copyright 2010, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ATOMIZER_H
#define _ATOMIZER_H

#include <SupportDefs.h>
#include <module.h>


#ifdef __cplusplus
extern "C" {
#endif


#define B_ATOMIZER_MODULE_NAME	"generic/atomizer/v1"
#define B_SYSTEM_ATOMIZER_NAME	"Haiku System Atomizer"


typedef struct atomizer_info {
	void*		atomizer;
	char		name[B_OS_NAME_LENGTH];
	uint32		atom_count;
} atomizer_info;


typedef struct atomizer_module_info {
	module_info		minfo;
	const void*		(*find_or_make_atomizer)(const char* string);
	status_t		(*delete_atomizer)(const void* atomizer);
	const void*		(*atomize) 
						(const void* atomizer, const char* string, int create);
	const char*		(*string_for_token)
						(const void* atomizer, const void* atom);
	status_t		(*get_next_atomizer_info) 
						(void** cookie, atomizer_info* info);
	const void*		(*get_next_atom)(const void* atomizer, uint32* cookie);
} atomizer_module_info;


#ifdef __cplusplus
}
#endif


#endif	/* _ATOMIZER_H */
