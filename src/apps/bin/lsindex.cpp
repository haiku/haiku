//	lsindex - for OpenBeOS
//
//	authors, in order of contribution:
//	jonas.sundstrom@kirilla.com
//

// std C includes
#include <stdio.h>
#include <string.h>
#include <errno.h>

// BeOS C includes
#include <fs_info.h>
#include <fs_index.h>
#include <TypeConstants.h>

void	print_help	(void);
void	print_index_stat (const index_info * const a_index_info);
void	print_index_type (const index_info * const a_index_info);

int	main (int32 argc, char **argv)
{
	dev_t		vol_device			=	dev_for_path(".");
	DIR		*	dev_index_dir		=	NULL;
	bool		verbose				=	false;
	bool		mkindex_output			=	false; /* mkindexi-ready output */
	
	for (int i = 1;  i < argc;  i++)
	{
		if (! strcmp(argv[i], "--help"))
		{
			print_help();
			return (0);
		}
	
		if (argv[i][0] == '-')
		{
			if 	(! strcmp(argv[i], "--verbose"))
				verbose =	true;
			else if 	(! strcmp(argv[i], "--mkindex"))
				mkindex_output =	true;
			else
			{	
				fprintf(stderr, "%s: option %s is not understood (use --help for help)\n", argv[0], argv[i]);
				return (-1);
			}
		}
		else
		{
			vol_device	=	dev_for_path(argv[i]);
			
			if (vol_device < 0)
			{
				fprintf(stderr, "%s: can't get information about volume: %s\n", argv[0], argv[i]);
				return (-1);
			}
		}
	}
	
	dev_index_dir =	fs_open_index_dir(vol_device);
	
	if (dev_index_dir == NULL)
	{
		fprintf(stderr, "%s: can't open index dir of device %ld\n", argv[0], vol_device);
		return (-1);
	}

	if (verbose)
	{
		printf(" Name   Type   Size   Created   Modified   User   Group\n");
		printf("********************************************************\n");
	}

	while (1)
	{
		dirent	*	index_entry	=	fs_read_index_dir (dev_index_dir);
		if (index_entry == NULL)
		{
			if (errno != B_ENTRY_NOT_FOUND && errno != B_OK)
			{
				printf("%s: fs_read_index_dir: (%d) %s\n", argv[0], errno, strerror(errno));
				return (errno);
			}
			
			break;
		}


		if (verbose || mkindex_output)
		{
			index_info	i_info;
			status_t	status	=	B_OK;
			
			status	=	fs_stat_index(vol_device, index_entry->d_name, & i_info);
			
			if (status != B_OK)
			{
				printf("%s: fs_stat_index(): (%d) %s\n", argv[0], errno, strerror(errno));
				return (errno);
			}
			if (verbose) {
				printf("%s", index_entry->d_name);

				if (strlen(index_entry->d_name) < 8)
					printf("  ");

				printf("\t");
				print_index_stat (& i_info);

			} else { /* mkindex_output */
				printf("mkindex -t ");
				print_index_type(& i_info);
				printf(" '%s'\n", index_entry->d_name);
			}	
		}
		else
		{
			printf("%s\n", index_entry->d_name);
		}
	}
	
	fs_close_index_dir (dev_index_dir);

	return (0);
}

void print_help (void)
{
	fprintf (stderr, 
		"Usage: lsindex [--help | --verbose | --mkindex] [volume path]\n"
		"   --verbose gives index type, dates and owner,\n"
		"   --mkindex outputs mkindex commands to recreate all the indices,\n"
		"   If no volume is specified, the volume of the current directory is assumed.\n");
}

void print_index_type (const index_info * const a_index_info)
{
	// type
	switch (a_index_info->type)
	{
		case B_INT32_TYPE:
			printf("int");
			break;
		case B_INT64_TYPE:
			printf("llong");
			break;
		case B_STRING_TYPE:
			printf("string");
			break;
		case B_FLOAT_TYPE:
			printf("float");
			break;
		case B_DOUBLE_TYPE:
			printf("double");
			break;
		default:
			printf("0x%08lx", (unsigned) a_index_info->type);
	}
}

void print_index_stat (const index_info * const a_index_info)
{
	// type
	switch (a_index_info->type)
	{
		// all types from <TypeConstants.h> listed for completeness,
		// even though they don't all apply to attribute indices
		
		case B_ANY_TYPE:					printf("B_ANY_TYPE");					break;
		case B_BOOL_TYPE:					printf("B_BOOL_TYPE");					break;
		case B_CHAR_TYPE:					printf("B_CHAR_TYPE");					break;
		case B_COLOR_8_BIT_TYPE:			printf("B_COLOR_8_BIT_TYPE");			break;
		case B_DOUBLE_TYPE:				printf("B_DOUBLE_TYPE");				break;
		case B_FLOAT_TYPE:					printf("B_FLOAT_TYPE");					break;
		case B_GRAYSCALE_8_BIT_TYPE:		printf("B_GRAYSCALE_8_BIT_TYPE");		break;
		case B_INT64_TYPE:					printf("B_INT64_TYPE");					break;
		case B_INT32_TYPE:					printf("B_INT32_TYPE");					break;
		case B_INT16_TYPE:					printf("B_INT16_TYPE");					break;
		case B_INT8_TYPE:					printf("B_INT8_TYPE");					break;
		case B_MESSAGE_TYPE:				printf("B_MESSAGE_TYPE");				break;
		case B_MESSENGER_TYPE:				printf("B_MESSENGER_TYPE");				break;
		case B_MIME_TYPE:					printf("B_MIME_TYPE");					break;
		case B_MONOCHROME_1_BIT_TYPE:		printf("B_MONOCHROME_1_BIT_TYPE");		break;
		case B_OBJECT_TYPE:					printf("B_OBJECT_TYPE");				break;
		case B_OFF_T_TYPE:					printf("B_OFF_T_TYPE");					break;
		case B_PATTERN_TYPE:				printf("B_PATTERN_TYPE");				break;
		case B_POINTER_TYPE:				printf("B_POINTER_TYPE");				break;
		case B_POINT_TYPE:					printf("B_POINT_TYPE");					break;
		case B_RAW_TYPE:					printf("B_RAW_TYPE");					break;
		case B_RECT_TYPE:					printf("B_RECT_TYPE");					break;
		case B_REF_TYPE:					printf("B_REF_TYPE");					break;
		case B_RGB_32_BIT_TYPE:				printf("B_RGB_32_BIT_TYPE");			break;
		case B_RGB_COLOR_TYPE:				printf("B_RGB_COLOR_TYPE");				break;
		case B_SIZE_T_TYPE:					printf("B_SIZE_T_TYPE");				break;
		case B_SSIZE_T_TYPE:				printf("B_SSIZE_T_TYPE");				break;
		case B_STRING_TYPE:					printf("B_STRING_TYPE");				break;
		case B_TIME_TYPE:					printf("B_TIME_TYPE");					break;
		case B_UINT64_TYPE:					printf("B_UINT64_TYPE");				break;
		case B_UINT32_TYPE:					printf("B_UINT32_TYPE");				break;
		case B_UINT16_TYPE:					printf("B_UINT16_TYPE");				break;
		case B_UINT8_TYPE:					printf("B_UINT8_TYPE");					break;
		case B_MEDIA_PARAMETER_TYPE:		printf("B_MEDIA_PARAMETER_TYPE");		break;
		case B_MEDIA_PARAMETER_WEB_TYPE:	printf("B_MEDIA_PARAMETER_WEB_TYPE");	break;
		case B_MEDIA_PARAMETER_GROUP_TYPE:	printf("B_MEDIA_PARAMETER_GROUP_TYPE");	break;
		case B_ASCII_TYPE:					printf("B_ASCII_TYPE");					break;	
		
		default:	printf("%ld\t", a_index_info->type);	break;
	}
	
	printf("\t");

	// Size
	printf("%Ld\t", a_index_info->size);
	
	// Created
	char * created	=	new char [30];
	strftime(created, 30, "%Y-%m-%d %H:%M", localtime(& a_index_info->creation_time)); 
	printf("%s\t", created);
	delete [] created;
	
	// Modified
	char * modified	=	new char [30];
	strftime(modified, 30, "%Y-%m-%d %H:%M", localtime(& a_index_info->modification_time)); 
	printf("%s\t", modified);
	delete [] modified;

	// User
	printf("%d\t", a_index_info->uid);
	
	// Group
	printf("%d\t", a_index_info->gid);
	
	printf("\n");
}

