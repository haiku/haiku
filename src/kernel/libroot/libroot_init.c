/* 
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <user_runtime.h>


extern void __init__dlfcn(struct uspace_program_args const *args);
extern void __init__image(struct uspace_program_args const *args);


void initialize_before(image_id imageID, struct uspace_program_args const *args);

void
initialize_before(image_id imageID, struct uspace_program_args const *args)
{
	__init__dlfcn(args);
	__init__image(args);
}

