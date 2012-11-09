/*
File:       icns_debug.c
Copyright (C) 2001-2012 Mathew Eis <mathew@eisbox.net>

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
#ifdef ICNS_DEBUG
void bin_print_byte(int x)
{
   int n;
   for(n=0; n<8; n++)
   {
	if((x & 0x80) !=0)
	{
	printf("1");
	}
	else
	{
   	printf("0");
   	}
	if(n==3)
	{
	printf(" "); /* insert a space between nybbles */
	}
	x = x<<1;
   }
}

void bin_print_int(int x)
{
   int hi, lo;
   hi=(x>>8) & 0xff;
   lo=x&0xff;
   bin_print_byte(hi);
   printf(" ");
   bin_print_byte(lo);
}
#endif /* ifdef ICNS_DEBUG */


