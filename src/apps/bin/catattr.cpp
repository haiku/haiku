
// Author:	Sebastian Nozzi
// Created:	1st may 2002

// Modifications:
// (please include author, date, and description)

// mmu_man@sf.net: note the original one doesn't link to libbe or libstdc++

#include <SupportDefs.h>
#include <TypeConstants.h>
#include <Mime.h>

#include <Entry.h>
#include <Node.h>
#include <kernel/fs_attr.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

void catAttr( const char *attrName, const char *fileName );
void dumpRawData( const char *buffer, off_t size );
void putCharOrDot( uchar c );

// Used to hold argv[0] along the program
char *completeToolName;

int main( int argc, char *argv[] )
{
	int returnCode;
	
	completeToolName = argv[0];
	returnCode = 0;
	
	if( argc > 2 ) {
	
		int i;
		
		// Cat the attribute for every file given
		for( i = 2; i < argc; i++ )
			catAttr( argv[1], argv[i] );
					
	} else {
		// Issue usage message
		fprintf( stderr, "usage: %s attr_name file1 [file2...]\n", completeToolName );
		// Be's original version -only- returned 1 if the 
		// amount of parameters was wrong, not if the file
		// or attribute couldn't be found (!)
		// In all other cases it returned 0
		returnCode = 1;
	}
	
	return returnCode;
}


void catAttr( const char *attrName, const char *fileName ) {
	
	BNode node;
	// Traverse links
	BEntry entry(fileName, true);

	node.SetTo(&entry);
	// Release file descriptor, which are limited
	entry.Unset();
	
	if( node.InitCheck() == B_OK ){
		// I chose to lock the node just in case
		// If you feel this is too protective remove it
		if(	node.Lock() == B_OK ) {
			
			attr_info attrInfo;
			
			if( node.GetAttrInfo( attrName, &attrInfo ) == B_OK ) {
				char *rawBuffer;
				
				rawBuffer = (char*) malloc( attrInfo.size );
				
				if( rawBuffer != NULL ) {
					node.ReadAttr(	attrName, attrInfo.type, 0, 
									rawBuffer, attrInfo.size );
					switch( attrInfo.type ) {
						case B_INT32_TYPE:
							printf("%s : int32 : %ld\n", fileName, *((int32*)rawBuffer) );
							break;
						case B_INT64_TYPE:
							printf("%s : int64 : %lld\n", fileName, *((int64*)rawBuffer) );
							break;
						case B_FLOAT_TYPE:
							printf("%s : float : %f\n", fileName, *((float*)rawBuffer) );
							break;
						case B_DOUBLE_TYPE:
							printf("%s : double : %f\n", fileName, *((double*)rawBuffer) );
							break;
						case B_BOOL_TYPE:
							printf("%s : bool : %d\n", fileName, *((unsigned char*)rawBuffer) );
							break;
						case B_STRING_TYPE:
						case B_MIME_STRING_TYPE:
							printf("%s : string : %s\n", fileName, rawBuffer );
							break;
						default:
#ifdef DEBUG
// This is to see the four letters that build up the attribute type
// in case you are ever unsure of what's going on
// After you got it, go and compare it against the types in
// TypeDefinitions.h
	if(1){
		char x[4];
		memcpy( (void*)&x[0], (void*) &attrInfo.type, 4 );
		printf("%c%c%c%c\n",x[3],x[2],x[1],x[0]);
	}
#endif
							// The rest of the attributes types are displayed as raw data
							// Surprisingly for me, even types like B_UINT32_TYPE or 
							// B_CHAR_TYPE... As of now I will implement this tool exactly
							// as Be's original behaved, then we can see to change this
							printf("%s : raw_data : \n", fileName );
							dumpRawData( rawBuffer, attrInfo.size );
							break;
					} /* type-switch */
					free(rawBuffer);
				} else {
					// This message I thought myself. It doesn't correspond to Be's
					// original version
					fprintf( stderr, "Not enough memory to complete this operation\n" );
				}
			} else {
				// Although misleading, this is the message provided by Be's original
				// version if the attribute could not be found
				fprintf( stderr, "%s: file %s attribute %s: "
					"No such file or directory\n", completeToolName, fileName, attrName );
			}					
			node.Unlock();
		} else {
			// This message I also thought myself. It doesn't correspond to Be's
			// original version
			fprintf( stderr, "%s: unable lock file %s\n", completeToolName, fileName );
		}
	} else {
		fprintf( stderr, "%s: can\'t open file %s to cat attribute %s\n", 
			completeToolName, fileName, attrName );
	}
}

// Dumps the contents of the attribute in the form of
// raw data. This view is used for the type B_RAW_DATA_TYPE,
// for custom types and for any type that is not directly
// supported by the utility "addattr"
void dumpRawData( const char *buffer, off_t size ) {

	off_t chunkEnd;
	off_t dumpPosition;
	off_t textPosition;
	uchar c;
	
	chunkEnd = 0x10;
	dumpPosition= 0;
	
	while( dumpPosition < size ) {
		// Position for this line
		printf( "0x%06lx: ", (unsigned long)dumpPosition );

		textPosition = dumpPosition;

		// Print the bytes in form of hexadecimal numbers
		while( dumpPosition < chunkEnd ) {
			if( dumpPosition < size ){
				c = buffer[dumpPosition];
				printf( "%02x", c );
				if( dumpPosition+1 < size )
					putchar(',');
				else
					putchar(' ');
			} else {
				printf("   ");
			}				
			dumpPosition++;
		}
		
		// Print the bytes in form of printable characters
		// (whenever possible)
		printf(" \'");
		while( textPosition < chunkEnd ) {
			if( textPosition < size ){
				c = buffer[textPosition];
				putCharOrDot( c );
			} else {
				putchar(' ');
			}				
			textPosition++;
		}
		printf("\'\n");
		
		chunkEnd += 0x10;
	}/* loop */
} 

// Used to present the characters in the raw data view
void putCharOrDot( uchar c ) {
// By using this:

//	if( ! isgraph(c) )

// better results were archieved, the output was leaner
// but it's not the way it wokrs in Be's version. So I
// decided to let the "bug" there and maybe we can
// see what we do about it for R2

	// Put the dot for non-printable characters
	// (including and below space, which code is 32)
	if( c <= ' ' )
		putchar('.');
	else
		putchar(c);			
}




