/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Device Manager

	Expansion of patterns.
	Used to expand consumer/connector/device names etc.
*/


#include "device_manager_private.h"

#include <KernelExport.h>
#include <TypeConstants.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//#define TRACE_PATTERNS
#ifdef TRACE_PATTERNS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/** expand node attribute
 *	node		- node to get attributes from
 *	*pattern	- name of attribute, ending with "%" (takes care of "^" sequences)
 *				  on return, *patterns points to trailing "%"
 *	buffer		- buffer for name
 *	dst			- string to write content to
 *	node_lock must be hold
 */

static status_t
expand_attr(device_node_info *node, const char **pattern, char *buffer, char *dst)
{	
	device_attr_info *attr;
	const char *str;
	int buffer_len;

	// extract attribute name
	buffer_len = 0;

	for (str = *pattern; *str; ++str) {
		if (*str == '^' && *(str + 1) != 0)
			// copy escaped character directly
			++str;
		else if (*str == '%')
			break;

		buffer[buffer_len++] = *str;
	}

	*pattern = str;

	buffer[buffer_len] = '\0';

	// get attribute content and add it to path		
	attr = pnp_find_attr_nolock(node, buffer, true, B_ANY_TYPE);
	if (attr == NULL) {
		dprintf("Reference to unknown attribute %s\n", buffer);
		return B_NAME_NOT_FOUND;
	}

	switch (attr->attr.type) {
		case B_UINT8_TYPE:
			sprintf(buffer, "%02x", attr->attr.value.ui8);
			strlcat(dst, buffer, PATH_MAX);
			break;
		case B_UINT16_TYPE:
			sprintf(buffer, "%04x", attr->attr.value.ui16);
			strlcat(dst, buffer, PATH_MAX);
			break;
		case B_UINT32_TYPE:
			sprintf(buffer, "%08lx", attr->attr.value.ui32);
			strlcat(dst, buffer, PATH_MAX);
			break;
		case B_UINT64_TYPE:
			sprintf(buffer, "%16Lx", attr->attr.value.ui64);
			strlcat(dst, buffer, PATH_MAX);
			break;
		case B_STRING_TYPE: {
			const char *str;

			//strlcat(dst, "\"", PATH_MAX);

			for (str = attr->attr.value.string; *str; ++str) {
				char ch;

				ch = *str;
				if (ch >= 32 && ch <= 126 && ch != '/' && ch != '%' && ch != '"') {
					buffer[0] = ch;
					buffer[1] = 0;
				} else {
					// convert unprintable/forbidden characters to numbers
					sprintf(buffer, "%%%u%%", (unsigned char)ch);
				}

				strlcat(dst, buffer, PATH_MAX);
			}

			//strlcat(dst, "\"", PATH_MAX);
			break;
		}
		case B_RAW_TYPE:
		default:
			benaphore_unlock(&gNodeLock);
			return B_BAD_VALUE;
	}
	
	benaphore_unlock(&gNodeLock);
	return B_OK;
}


/** expand pattern.
 *	the pattern is both expanded and the position of inidiual parts returned
 *	pattern	  	- pattern to expand
 *	dest		  	- expanded name is appended to this string
 *	buffer		- buffer of MAX_PATH+1 size supplied by caller
 *	term_array	- list of lengths of individual sub-names
 *				  with index 0 containing the length of the shortest and 
 *	num_parts-1	- length of the longest sub-name
 *	<term_array> and <num_parts> can be NULL
 *	node_lock must be hold
 */

status_t
pnp_expand_pattern(device_node_info *node, const char *pattern, char *dest,
	char *buffer, size_t *term_array, int *num_parts)
{
	const char *str;
	int id;
	status_t res;
	int buffer_len = 0;

	TRACE(("expand_pattern(pattern: %s)\n", pattern));

	for (str = pattern, id = 0; *str; ++str) {
		switch (*str) {
			case '^':
				// character following "^" is copied directly
				// exception: if there is a trailing "^" at end of string,
				// we copy it directly
				if (*(str + 1) != 0) 
					++str;

				buffer[buffer_len++] = *str;
				break;

			case '|':
				// end of one name part
				buffer[buffer_len] = '\0';
				buffer_len = 0;
				strlcat(dest, buffer, PATH_MAX);

				TRACE(("%d: %s\n", id, dest));

				if (term_array != NULL)
					term_array[id++] = strlen(dest);
				break;

			case '%':
				// begin of place-holder
				buffer[buffer_len] = '\0';
				buffer_len = 0;
				strlcat(dest, buffer, PATH_MAX);

				// skip "%"
				++str;

				res = expand_attr(node, &str, buffer, dest);
				if (res != B_OK)
					return res;
				break;

			default:
				// normal character
				buffer[buffer_len++] = *str;
		}

		// avoid overflow of buffer		
		if (buffer_len > PATH_MAX)
			buffer_len = PATH_MAX;
	}

	// append trailing characters
	buffer[buffer_len] = 0;
	buffer_len = 0;	
	strlcat(dest, buffer, PATH_MAX);
	
	if (term_array != NULL)
		term_array[id++] = strlen(dest);

	if (num_parts != NULL)
		*num_parts = id;

	TRACE(("result: %s\n", dest));

	return B_OK;
}


/** expand pattern as specified by attribute <attr_name>
 *	node_lock must be hold
 */

status_t
pnp_expand_pattern_attr(device_node_info *node, const char *attr_name, 
	char **expanded)
{
	const char *pattern;
	status_t res;

	TRACE(("pnp_expand_pattern_attr()\n"));

	if (pnp_get_attr_string_nolock(node, attr_name, &pattern, false) != B_OK) {
		*expanded = strdup("");
		return B_OK;
	}

	*expanded = malloc((PATH_MAX + 1) * 2);
	if (*expanded == NULL)
		return B_NO_MEMORY;

	**expanded = 0;
	res = pnp_expand_pattern(node, pattern, 
		*expanded, *expanded + PATH_MAX + 1, NULL, NULL);

	if (res != B_OK)
		free(*expanded);

	return res;
}
