
// Author:	Sebastian Nozzi
// Created:	1st may 2002

// Modifications:
// (please include author, date, and description)

#include <Mime.h>
#include <SupportDefs.h>
#include <TypeConstants.h>

#include <stdio.h>
#include <string.h>

#include "addAttr.h"
#include "messages.h"

status_t typeForString( const char *attrSting, type_code *result );

int main( int argc, char*argv[] )
{
	int returnCode;
	
	completeToolName = argv[0];
	returnCode = 0;

	if( argc > 1 ) {
		type_code attrType;
		unsigned minArguments;
		bool usingDefaultAttr;
		bool validAttrType;
		
		usingDefaultAttr = true;
		validAttrType = true;
		minArguments = 3;
		
		// Get the attribute type
		if( strcmp( argv[1], "-t" ) == 0 ) {
			if( argc == 2 ) {
				invalidAttrMsg( "(null)" );
				validAttrType = false;
				returnCode = 1;
			} else if( typeForString( argv[2], &attrType ) == B_BAD_VALUE )	{
				invalidAttrMsg( argv[2] );
				validAttrType = false;
				returnCode = 1;
			} else {
				usingDefaultAttr = false;
				minArguments = 5;
			}
		} else {
			// The user didn't specify a type
			// The default is 'string'
			attrType = B_STRING_TYPE;
		}
		
		if( validAttrType ) {		
			if( (unsigned)argc > minArguments ) {
				char **files;
				char *attrName;
				char *attrValue;
		
				// The location of the rest of the argument 
				// varies depending on wether there was a
				// "-t" switch or not
				if( usingDefaultAttr ) {
					attrName = argv[1];
					attrValue = argv[2];
					files = &(argv[3]);
				} else {
					attrName = argv[3];
					attrValue = argv[4];
					files = &(argv[5]);
				}				
				
				// Now that we gathered all the information proceed
				// to add the attribute to the file(s)
				addAttrToFiles( attrType, attrName, attrValue, 
								files, argc-minArguments );
				
			} else { // some arguments are missing
				usageMsg();
				returnCode = 1;
			}
		}
		
	} else { // called with no arguments
		usageMsg();
		returnCode = 1;
	}

	return returnCode;
}

// For the given string that the user specifies as attribute type
// in the command line, this function tries to figure out the
// corresponding Be API value
//
// On success, "result" will contain that value
// On failure, B_BAD_VALUE is returned and "result" is not modified
status_t typeForString( const char *attrString, type_code *result )
{
	status_t returnCode;
	
	returnCode = B_OK;
	
	if( strcmp( attrString, "string" ) == 0 ){
		*result = B_STRING_TYPE;
	} else if( strcmp( attrString, "mime" ) == 0 ){
		*result = B_MIME_STRING_TYPE;
	} else if( strcmp( attrString, "int" ) == 0 ) {
		*result = B_INT32_TYPE;
	} else if( strcmp( attrString, "llong" ) == 0 ) {
		*result = B_INT64_TYPE;
	} else if( strcmp( attrString, "float" ) == 0 ) {
		*result = B_FLOAT_TYPE;
	} else if( strcmp( attrString, "double" ) == 0 ) {
		*result = B_DOUBLE_TYPE;
	} else if( strcmp( attrString, "bool" ) == 0 ) {
		*result = B_BOOL_TYPE;
	} else {
		int convertedFields;
		type_code tempCode;
		
		convertedFields = sscanf( attrString, "%lu", &tempCode );
		
		if( convertedFields > 0 ) {
			*result = tempCode;
		} else {
			returnCode = B_BAD_VALUE;
		}
	}

	return returnCode;
}

