/*
 * finddir.c - 
 * (c) 2002, Sebastian Nozzi <sebnozzi@gmx.net>
 */

#include <FindDirectory.h>
#include <fs_info.h>

#include <stdio.h>
#include <string.h>

typedef struct
{
	const char *key;
	directory_which value;
} directoryType;

#define KEYVALUE_PAIR(key) {#key,key}

directoryType directoryTypes[] = {
	KEYVALUE_PAIR(B_DESKTOP_DIRECTORY),
	KEYVALUE_PAIR(B_TRASH_DIRECTORY),
	KEYVALUE_PAIR(B_APPS_DIRECTORY),
	KEYVALUE_PAIR(B_PREFERENCES_DIRECTORY),
	KEYVALUE_PAIR(B_BEOS_DIRECTORY),
	KEYVALUE_PAIR(B_BEOS_SYSTEM_DIRECTORY),
	KEYVALUE_PAIR(B_BEOS_ADDONS_DIRECTORY),
	KEYVALUE_PAIR(B_BEOS_BOOT_DIRECTORY),
	KEYVALUE_PAIR(B_BEOS_FONTS_DIRECTORY),
	KEYVALUE_PAIR(B_BEOS_LIB_DIRECTORY),
	KEYVALUE_PAIR(B_BEOS_SERVERS_DIRECTORY),
	KEYVALUE_PAIR(B_BEOS_APPS_DIRECTORY),
	KEYVALUE_PAIR(B_BEOS_BIN_DIRECTORY),
	KEYVALUE_PAIR(B_BEOS_ETC_DIRECTORY),
	KEYVALUE_PAIR(B_BEOS_DOCUMENTATION_DIRECTORY),
	KEYVALUE_PAIR(B_BEOS_PREFERENCES_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_SYSTEM_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_ADDONS_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_BOOT_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_FONTS_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_LIB_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_SERVERS_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_BIN_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_ETC_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_DOCUMENTATION_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_SETTINGS_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_DEVELOP_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_LOG_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_SPOOL_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_TEMP_DIRECTORY),
	KEYVALUE_PAIR(B_COMMON_VAR_DIRECTORY),
	KEYVALUE_PAIR(B_USER_DIRECTORY),
	KEYVALUE_PAIR(B_USER_CONFIG_DIRECTORY),
	KEYVALUE_PAIR(B_USER_ADDONS_DIRECTORY),
	KEYVALUE_PAIR(B_USER_BOOT_DIRECTORY),
	KEYVALUE_PAIR(B_USER_FONTS_DIRECTORY),
	KEYVALUE_PAIR(B_USER_LIB_DIRECTORY),
	KEYVALUE_PAIR(B_USER_SETTINGS_DIRECTORY),
	KEYVALUE_PAIR(B_USER_DESKBAR_DIRECTORY),
	{NULL,B_USER_DESKBAR_DIRECTORY}
};

bool retrieveDirValue( directoryType *list, const char *key, directory_which *value_out )
{
	unsigned i = 0;
	
	while( list[i].key != NULL )
	{
		if( strcmp( list[i].key, key ) == 0 )
		{
			*value_out = list[i].value;
			return true;
		}
		
		i++;
	}
	
	return false;
}

void usageMsg(){
	printf("usage:  /bin/finddir [ -v volume ] directory_which\n" );
	printf("\t-v <file>   use the specified volume for directory\n" );
	printf("\t\t    constants that are volume-specific.\n");
	printf("\t\t    <file> can be any file on that volume.\n");
	printf("\t\t    defaults to the boot volume.\n");
	printf(" For a description of recognized directory_which constants,\n");
	printf(" see the find_directory(...) documentation in the Be Book.\n");
}

#define NO_ERRORS   0
#define ARGUMENT_MISSING 1
#define WRONG_DIR_TYPE  2

int main(int argc, char *argv[])
{
	int directoryArgNr;
	int status;
	dev_t volume;
	directory_which dirType;
	int returnCode;
	
	status = NO_ERRORS;
	directoryArgNr = 1;
	returnCode = 0;

	dirType = B_BEOS_DIRECTORY; /* so that it compiles */
	
	/* By default use boot volume*/
	volume = dev_for_path( "/boot" );
	
	if( argc <= 1 ){
		status = ARGUMENT_MISSING;
	} else {
		if( strcmp(argv[1], "-v") == 0 ){
			if( argc >= 3 ){
				dev_t temp_volume;
				/* get volume from second arg */
				temp_volume = dev_for_path( argv[2] );
				
				/* Keep default value in case of error */
				if( temp_volume >= 0 )
					volume = temp_volume;
				
				/* two arguments were used for volume */
				directoryArgNr+=2;
			} else {
				/* set status to argument missing */
				status = ARGUMENT_MISSING;
			}
		}
	}
	
	if( status == NO_ERRORS && argc > directoryArgNr ){
		/* get directory constant from next argument */

		if( retrieveDirValue( directoryTypes, argv[directoryArgNr], &dirType ) == false ){
			status = WRONG_DIR_TYPE;
		}
		
	} else {
		status = ARGUMENT_MISSING;
	}
	
	/* Do the actual directoy finding */
	
	if( status == NO_ERRORS ){
		/* Question: would B_PATH_NAME_LENGTH alone have been enough? */
		char buffer[B_PATH_NAME_LENGTH+B_FILE_NAME_LENGTH];
		status_t result;
		
		result = find_directory( dirType, volume, /*create it*/ false, buffer, sizeof(buffer) );
		
		if( result == B_OK ){
			printf( "%s\n", buffer );
		}else{
			/* else what? */
			/* this can not happen! */
			printf("Serious internal error; contact support\n");
		}
	}

	/* Error messages and return code setting */

	if( status == WRONG_DIR_TYPE )	{
		printf("%s: unrecognized directory_which constant \'%s\'\n", argv[0], argv[directoryArgNr] );
		returnCode = 252;
	}

	if( status == ARGUMENT_MISSING ){
		usageMsg();
		returnCode = 255;
	}
	
	return returnCode;
}
