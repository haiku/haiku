/*! \file structs.h
    \brief Holds the structs used.
    
*/

#ifndef STRUCTS_H

	#define STRUCTS_H
	
	struct partition_info{
	
		int32 type;			/* partition type */
		/* Maybe get this from the type mapping? */
		char *fs;		/* file system */
		char *name;		/* volume name */
		char *mount_pt;	/* mounted at */
		double size;		/* size (in MB) */
		bool mounted;	/* TRUE if mounted, FALSE if not */
	
	};
	
	struct dev_info{
	
		char *device;   /* the path to the device */
		int map;			/* 0 for none, 1 for intel, 2 for apple */
		int numParts;	/* number of partitions */
		partition_info parts[4];	/* information on the partitions on the device */
		
	};
	
#endif

