//	df - for OpenBeOS
//
//	authors, in order of contribution:
//	jonas.sundstrom@kirilla.com
//

#include <stdio.h>
#include <string.h>

#include <String.h>
#include <fs_info.h>
#include <fs_index.h>

#include <Volume.h>
#include <Directory.h>
#include <Path.h>


void	PrintUsageInfo	(void);
void	PrintFlagSupport (uint32 dev_flags, uint32 test_flag, char * flag_descr, bool verbose);
void	PrintMountPoint (dev_t dev_num, bool verbose);
void	PrintType (const char * fsh_name);
void	PrintBlocks (int64 blocks, int64 blocksize, bool human);

int	main (int32 argc, char **argv)
{
	bool verbose	=	false;
	bool human	=	false;

	if (argc > 1)	
	{
		BString option	=	argv[1];
		option.ToLower();
		
		if (option == "--help")
		{
			PrintUsageInfo();
			return (0);
		}
		
		if (option == "--verbose")
		{
			verbose	=	true;
		}
		
		if (option == "--human" || option == "-h")
		{
			human =		true;
		}
	}

	if (! verbose)
	{
		printf( "Mount            Type     Total    Free     Flags   Device\n"
				"---------------- -------- -------- -------- ------- --------------------------\n");
	}
	
	dev_t		device_num	=	0;
	int32		cookie		=	0;
	fs_info		info;

	while (1)
	{
		device_num =	next_dev(& cookie);
		
		if (device_num != B_BAD_VALUE)
		{	
			if(fs_stat_dev(device_num, &info) == B_OK)
			{
				if (verbose)
				{
					PrintMountPoint (info.dev, verbose);
					
					printf("device: %ld\n",			info.dev);
					printf("volume_name: %s\n",		info.volume_name);
					printf("fsh_name: %s\n",		info.fsh_name);
					printf("device_name: %s\n",		info.device_name);
					printf("flags: %lu\n",			info.flags);

					PrintFlagSupport (info.flags, B_FS_HAS_MIME, "mimetypes:", verbose);
					PrintFlagSupport (info.flags, B_FS_HAS_ATTR, "attributes:", verbose);
					PrintFlagSupport (info.flags, B_FS_HAS_QUERY, "queries:", verbose);
					PrintFlagSupport (info.flags, B_FS_IS_READONLY, "readonly:", verbose);
					PrintFlagSupport (info.flags, B_FS_IS_REMOVABLE, "removable:", verbose);
					PrintFlagSupport (info.flags, B_FS_IS_PERSISTENT, "persistent:", verbose);
					PrintFlagSupport (info.flags, B_FS_IS_SHARED, "shared:", verbose);
					
					printf("block_size: %Ld\n",		info.block_size);
					printf("io_size: %Ld\n",		info.io_size);
					printf("total_blocks: %Ld\n",	info.total_blocks);
					printf("free_blocks: %Ld\n",	info.free_blocks);
					printf("total_nodes: %Ld\n",	info.total_nodes);
					printf("free_nodes: %Ld\n",		info.free_nodes);
					printf("root inode: %Ld\n",		info.root);	
					
					
					printf("--------------------------------\n");
				}
				else // compact, default mode
				{
					PrintMountPoint (info.dev, verbose);
					PrintType(info.fsh_name);
					PrintBlocks(info.total_blocks, info.block_size, human);
					PrintBlocks(info.free_blocks, info.block_size, human);

					PrintFlagSupport (info.flags, B_FS_HAS_QUERY, "Q", verbose);
					PrintFlagSupport (info.flags, B_FS_HAS_ATTR, "A", verbose);
					PrintFlagSupport (info.flags, B_FS_HAS_MIME, "M", verbose);
					PrintFlagSupport (info.flags, B_FS_IS_SHARED, "S", verbose);
					PrintFlagSupport (info.flags, B_FS_IS_PERSISTENT, "P", verbose);
					PrintFlagSupport (info.flags, B_FS_IS_REMOVABLE, "R", verbose);
					PrintFlagSupport (info.flags, B_FS_IS_READONLY, NULL, verbose);

					printf(" ");
					printf("%s\n",		info.device_name);
					
				}
			}
		}
		else
			break;
	}

	return (0);
}

void PrintUsageInfo (void)
{
	printf ("usage: df [--verbose | --help | --human | -h]\n"
			"  -h = --human = human readable (KB by default)\n"
			"  flags:\n"
			"   Q: has query\n"
			"   A: has attribute\n"
			"   M: has mime\n"
			"   S: is shared\n"
			"   R: is removable\n"
			"   W: is writable\n"
			);
}

void PrintFlagSupport (uint32 dev_flags, uint32 test_flag, char * flag_descr, bool verbose)
{
	if (verbose)
	{
		if	(dev_flags & test_flag)
			printf("%s yes\n", flag_descr);
		else
			printf("%s no\n", flag_descr);
	}
	else
	{
		if (test_flag == B_FS_IS_READONLY)
		{
			if	(dev_flags & test_flag)
				printf("-");
			else
				printf("W");
			return;
		}
	
		if	(dev_flags & test_flag)
			printf("%s", flag_descr);
		else
			printf("-");
	}
}

void PrintMountPoint (dev_t dev_num, bool verbose)
{
	BString		mounted_at	=	"                 ";
	BVolume		volume (dev_num);
	BDirectory	vol_dir;
	volume.GetRootDirectory(& vol_dir);
	BPath	path	(& vol_dir, NULL);
	mounted_at.Insert(path.Path(), 0);

	if (verbose)
	{
		printf("Mounted at: %s\n", mounted_at.String());
	}
	else
	{
		mounted_at.Truncate(17);
		printf("%s", mounted_at.String());
	}
}

void PrintType (const char * fsh_name)
{
	BString		type	=	"         ";
	type.Insert(fsh_name, 0);

	type.Truncate(9);
	printf("%s", type.String());
}

void PrintBlocks (int64 blocks, int64 blocksize, bool human)
{
	char * temp	=	new char [1024];
	if (!human)
		sprintf(temp, "%lld", blocks * (blocksize / 1024));
	else {
		if (blocks < 1024) {
			sprintf(temp, "%lld" /*" B"*/, blocks);
		} else {
			double fblocks = ((double)blocks) * (blocksize / 1024);
			if (fblocks < 1024) {
				sprintf(temp, "%.1LfK", fblocks);
			} else {
				fblocks = (((double)blocks) / 1024) * (blocksize / 1024);;
				if (fblocks < 1024) {
					sprintf(temp, "%.1LfM", fblocks);
				} else {
					fblocks = (((double)blocks) / (1024*1024)) * (blocksize / 1024);
					sprintf(temp, "%.1LfG", fblocks);
				}
			}
		}
	}
	
	BString		type	=	"         ";
	type.Insert(temp, 8 - strlen(temp));

	type.Truncate(9);
	printf("%s", type.String());

	delete [] temp;
}

