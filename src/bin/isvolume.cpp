//	isvolume - for OpenBeOS
//
//	authors, in order of contribution:
//	jonas.sundstrom@kirilla.com
//

// std C includes
#include <stdio.h>
#include <string.h>

// BeOS C includes
#include <fs_info.h>

void	print_help	(void);

int	main (int32 argc, char **argv)
{
	dev_t		vol_device			=	dev_for_path(".");
	uint32		is_volume_flags		=	0;
	fs_info		volume_info;
	
	for (int i = 1;  i < argc;  i++)
	{
		if (! strcmp(argv[i], "--help"))
		{
			print_help();
			return (0);
		}
	
		if (argv[i][0] == '-')
		{
			if 		(! strcmp(argv[i], "-readonly"))	is_volume_flags	|=	B_FS_IS_READONLY;
			else if (! strcmp(argv[i], "-query"))		is_volume_flags	|=	B_FS_HAS_QUERY;
			else if (! strcmp(argv[i], "-attribute"))	is_volume_flags	|=	B_FS_HAS_ATTR;
			else if (! strcmp(argv[i], "-mime"))		is_volume_flags	|=	B_FS_HAS_MIME;
			else if (! strcmp(argv[i], "-shared"))		is_volume_flags	|=	B_FS_IS_SHARED;
			else if (! strcmp(argv[i], "-persistent"))	is_volume_flags	|=	B_FS_IS_PERSISTENT;
			else if (! strcmp(argv[i], "-removable"))	is_volume_flags	|=	B_FS_IS_REMOVABLE;
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
	
	if(fs_stat_dev(vol_device, & volume_info) == B_OK)
	{
		if	(volume_info.flags & is_volume_flags)
			printf("yes\n");
		else
			printf("no\n");

		return (0);
	}
	else
	{
		fprintf(stderr, "%s: can't get information about dev_t: %ld\n", argv[0], vol_device);
		return (-1);
	}
}

void print_help (void)
{
	fprintf (stderr, 
		"Usage: isvolume {-OPTION} [volumename]\n"
		"   Where OPTION is one of:\n"
		"           -readonly   - volume is read-only\n"
		"           -query      - volume supports queries\n"
		"           -attribute  - volume supports attributes\n"
		"           -mime       - volume supports MIME information\n"
		"           -shared     - volume is shared\n"
		"           -persistent - volume is backed on permanent storage\n"
		"           -removable  - volume is on removable media\n"
		"   If the option is true for the named volume, 'yes' is printed\n"
		"   and if the option is false, 'no' is printed. Multiple options\n"
		"   can be specified in which case all of them must be true.\n\n"
		"   If no volume is specified, the volume of the current directory is assumed.\n");
}

