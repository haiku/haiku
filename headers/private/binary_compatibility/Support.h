/*
 * Copyright 2010, Haiku, Inc. 
 * Distributed under the terms of the MIT License.
 */
#ifndef _BINARY_COMPATIBILITY_SUPPORT_H_
#define _BINARY_COMPATIBILITY_SUPPORT_H_


#include <binary_compatibility/Global.h>


struct perform_data_all_unarchived {
	const BMessage*	archive;
	status_t		return_value;
};


struct perform_data_all_archived {
	BMessage*	archive;
	status_t	return_value;
};

#endif /* _BINARY_COMPATIBILITY_SUPPORT_H_ */
