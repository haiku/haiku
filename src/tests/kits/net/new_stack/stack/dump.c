#include <stdio.h>

#include <drivers/module.h>
#include <drivers/KernelExport.h>

#include "dump.h"

// --------------------------------------------------
void dump_memory
	(
	const char *	prefix,
	const void *	data,
	size_t			len
	)
{
	uint32	i,j;
  	char	text[96];	// only 3*16 + 16 max by line needed
	uint8 *	byte;
	char *	ptr;

	byte = (uint8 *) data;

	for ( i = 0; i < len; i += 16 )
		{
		ptr = text;

      	for ( j = i; j < i+16 ; j++ )
			{
			if ( j < len )
				sprintf(ptr, "%02x ", byte[j]);
			else
				sprintf(ptr, "   ");
			ptr += 3;
			};
			
		for (j = i; j < len && j < i+16;j++)
			{
			if ( byte[j] >= 0x20 && byte[j] < 0x7e )
				*ptr = byte[j];
			else
				*ptr = '.';
				
			ptr++;
			};
		*ptr = '\n';
		ptr++;
		*ptr = '\0';
		
		if (prefix)
			dprintf(prefix);
		dprintf(text);

		// next line
		};
}
