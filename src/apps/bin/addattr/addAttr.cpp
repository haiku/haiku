// Author:      Sebastian Nozzi
// Created:     3 may 2002

// Modifications:
// (please include author, date, and description)

// mmu_man@sf.net: note the original one doesn't link to libbe


#include <TypeConstants.h>
#include <SupportDefs.h>
#include <Mime.h>
#include <Entry.h>
#include <Node.h>

#include <stdio.h>

#include "addAttr.h"
#include "messages.h"

// Adds a certain attribute to many files, specified in an array 
// of strings
// 
// On success it will return B_OK
// On failure it returns an error code (negative number)
status_t addAttrToFiles( 	type_code attrType, 
							const char *attrName, 
							const char *attrValue, 
							char **files, 
							unsigned fileCount ) {

	status_t returnCode;
	status_t result;
	unsigned fileIdx;

	returnCode = B_OK;

	// Iterate through all the files and add the attribute
	// to each of them

	for( fileIdx = 0; fileIdx < fileCount; fileIdx++ ) {
		result = addAttr( attrType, attrName, attrValue, files[fileIdx] );
		// Try to keep the first error code, if any
		if( returnCode == B_OK && result != B_OK )
			returnCode = result;
	}
	
	return returnCode;
}

// Adds an attribute to a file for the given type, name and value
// Locks and unlocks the corresponding node to avoid data inconsistence
// Converts the value accordingly in case of numeric or boolean types
// 
// On success it will return the amount of bytes writen
// On failure it returns an error code (negative number)
status_t addAttr( 	type_code attrType, 
					const char *attrName, 
					const char *attrValue, 
					const char *file ) {

	status_t returnCode;

	BNode node;
	// Traverse links
	BEntry entry(file, true);
	
	node.SetTo(&entry);
	// Release file descriptor
	entry.Unset();
	
#ifdef DEBUG
	printf("%lu | %s | %s | %s\n", attrType, attrName, attrValue, file );		
#endif

	returnCode = node.InitCheck();
	
	if( returnCode == B_OK ) {

		returnCode = node.Lock();	// to avoid data inconsistency

		if( returnCode == B_OK ) {
			// Only add the attribute if not already there
			if( hasAttribute( &node, attrName ) == false ) {

				ssize_t bytesWrittenCode;

				// Write the attribute now
				bytesWrittenCode = writeAttr( &node, attrType, attrName, attrValue );
				
				// If negative, then it's an error code
				if( bytesWrittenCode < 0 ) {
					problemsWithFileMsg( file );
					returnCode = bytesWrittenCode;
				}
			}	
			node.Sync();
			node.Unlock();
		} else { // Node could not be locked
			problemsWithFileMsg( file );
		}
	} else { 
		// File could not be initialized
		// (maybe it doesn't exist)
		problemsWithFileMsg( file );
	}
		
	return returnCode;
}

// Writes an attribute to a node, taking the type into account and
// convertig the value accordingly
// 
// On success it will return the amount of bytes writen
// On failure it returns an error code (negative number)
ssize_t writeAttr( 	BNode *node, type_code attrType, 
					const char *attrName, const char *attrValue ) {

	int32 int32buffer = 0;
	int64 int64buffer = 0;
	int boolBuffer = 0;
	float floatBuffer = 0.0;
	double doubleBuffer = 0.0;

	ssize_t bytesWrittenOrErrorCode;

	switch( attrType ) {
		case B_INT32_TYPE:
			sscanf( attrValue, "%ld", &int32buffer );
			bytesWrittenOrErrorCode = 
				node->WriteAttr( attrName, attrType, 0, &int32buffer, sizeof(int32) );
			break;
		case B_INT64_TYPE:
			sscanf( attrValue, "%lld", &int64buffer );
			bytesWrittenOrErrorCode = 
				node->WriteAttr( attrName, attrType, 0, &int64buffer, sizeof(int64) );
			break;
		case B_FLOAT_TYPE:
			sscanf( attrValue, "%f", &floatBuffer );
			bytesWrittenOrErrorCode = 
				node->WriteAttr( attrName, attrType, 0, &floatBuffer, sizeof(float) );
			break;
		case B_DOUBLE_TYPE:
			sscanf( attrValue, "%lf", &doubleBuffer );
			bytesWrittenOrErrorCode = 
				node->WriteAttr( attrName, attrType, 0, &doubleBuffer, sizeof(double) );
			break;
		case B_BOOL_TYPE:
			// NOTE: the conversion from "int" to "signed char" might seem strange
			// but this is the only way I could think to replicate Be's addattr behaviour
			sscanf( attrValue, "%d", &boolBuffer );
			boolBuffer = (signed char) boolBuffer;
			bytesWrittenOrErrorCode = 
				node->WriteAttr( attrName, attrType, 0, &boolBuffer, sizeof(signed char) );
			break;
		case B_STRING_TYPE:
		case B_MIME_STRING_TYPE:
		default:
			// For string, mime-strings and any other type we just write the value
			// NOTE that the trailing NULL -IS- added
			bytesWrittenOrErrorCode = 
				node->WriteAttr( attrName, attrType, 0, attrValue, strlen(attrValue)+1 );
			break;
	};

	return bytesWrittenOrErrorCode;
}

// Checks wether a node has an attribute under the given name or not
//
// The user is responsible for locking and unlocking the node, and
// for assuring that we are retrieving all attributes from the beginning
// by calling RewindAttrs() on the node, if necesary
bool hasAttribute( BNode *node, const char *attrName ) {

	char retrievedName[B_ATTR_NAME_LENGTH];
	bool found;
	
	found = false;
	
	while( (!found) && (node->GetNextAttrName( retrievedName )==B_OK) ) {
		if( strcmp( retrievedName, attrName ) == 0 )
			found = true;
	}

	return found;
}

