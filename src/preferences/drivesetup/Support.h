/*
 * Copyright 2002-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef SUPPORT_H
#define SUPPORT_H


#include <SupportDefs.h>


class BPartition;


const char* string_for_size(off_t size, char *string);

void dump_partition_info(const BPartition* partition);


#endif // SUPPORT_H
