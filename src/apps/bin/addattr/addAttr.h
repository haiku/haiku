// Author:      Sebastian Nozzi
// Created:     3 may 2002

// Modifications:
// (please include author, date, and description)

// mmu_man@sf.net: note the original one doesn't link to libbe

#ifndef _ADD_ATTR_H
#define _ADD_ATTR_H

#include <StorageDefs.h>

status_t addAttrToFiles( 	type_code attrType, 
							const char *attrName, 
							const char *attrValue, 
							char **files, 
							unsigned fileCount );

status_t addAttr( 	type_code attrType, 
					const char *attrName, 
					const char *attrValue, 
					const char *file );
					
bool hasAttribute( BNode *node, const char *attrName );
ssize_t writeAttr( 	BNode *node, type_code attrType, 
					const char *attrName, const char *attrValue );
					

#endif