/*
File:       icns_family.c
Copyright (C) 2001-2012 Mathew Eis <mathew@eisbox.net>
              2007 Thomas Lübking <thomas.luebking@web.de>
              2002 Chenxiao Zhao <chenxiao.zhao@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the
Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
Boston, MA 02110-1301, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "icns.h"
#include "icns_internals.h"

/***************************** icns_create_family **************************/

int icns_create_family(icns_family_t **iconFamilyOut)
{
	icns_family_t	*newIconFamily = NULL;
	icns_type_t		iconFamilyType = ICNS_NULL_TYPE;
	icns_size_t		iconFamilySize = 0;

	if(iconFamilyOut == NULL)
	{
		icns_print_err("icns_create_family: icon family reference is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	*iconFamilyOut = NULL;
	
	iconFamilyType = ICNS_FAMILY_TYPE;
	iconFamilySize = sizeof(icns_type_t) + sizeof(icns_size_t);

	newIconFamily = malloc(iconFamilySize);
		
	if(newIconFamily == NULL)
	{
		icns_print_err("icns_create_family: Unable to allocate memory block of size: %d!\n",iconFamilySize);
		return ICNS_STATUS_NO_MEMORY;
	}
	
	ICNS_WRITE_UNALIGNED(&(newIconFamily->resourceType), iconFamilyType, sizeof(icns_type_t));
	ICNS_WRITE_UNALIGNED(&(newIconFamily->resourceSize), iconFamilySize, sizeof(icns_size_t));

	*iconFamilyOut = newIconFamily;
	
	return ICNS_STATUS_OK;
}


/***************************** icns_count_elements_in_family **************************/

int icns_count_elements_in_family(icns_family_t *iconFamily, icns_sint32_t *elementTotal)
{
	icns_type_t            iconFamilyType = ICNS_NULL_TYPE;
	icns_size_t            iconFamilySize = 0;
	icns_uint32_t          dataOffset = 0;
	icns_uint32_t          elementCount = 0;

	if(iconFamily == NULL)
	{
		icns_print_err("icns_count_elements_in_family: icns family is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}

	if(elementTotal == NULL)
	{
		icns_print_err("icns_count_elements_in_family: element count ref is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}

	ICNS_READ_UNALIGNED(iconFamilyType, &(iconFamily->resourceType),sizeof( icns_type_t));
	ICNS_READ_UNALIGNED(iconFamilySize, &(iconFamily->resourceSize),sizeof( icns_size_t));
	
	dataOffset = sizeof(icns_type_t) + sizeof(icns_size_t);
	
	while( dataOffset < iconFamilySize )
	{
		icns_element_t	       *iconElement = NULL;
		icns_size_t            elementSize = 0;

		iconElement = ((icns_element_t*)(((char*)iconFamily)+dataOffset));
		ICNS_READ_UNALIGNED(elementSize, &(iconElement->elementSize),sizeof( icns_size_t));
		
		elementCount++;

		dataOffset += elementSize;
	}

	*elementTotal = elementCount;

	return ICNS_STATUS_OK;
}


