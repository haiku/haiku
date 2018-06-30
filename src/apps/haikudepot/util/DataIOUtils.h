/*
 * Copyright 2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef DATA_IO_UTILS_H
#define DATA_IO_UTILS_H


#include <DataIO.h>


class DataIOUtils {

public:
	static	status_t		Copy(BDataIO* target, BDataIO* source, size_t size);

};


#endif // DATA_IO_UTILS_H
