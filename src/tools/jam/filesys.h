/*
 * Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * filesys.h - OS specific file routines 
 */

typedef void (*scanback)( void *closure, char *file, int found, time_t t );

void file_dirscan( char *dir, scanback func, void *closure );
void file_archscan( char *arch, scanback func, void *closure );

int file_time( char *filename, time_t *time );
