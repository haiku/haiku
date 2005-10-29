/* $Id: bootdir.h 3132 2003-05-03 03:43:30Z axeld $
**
** Copyright 1998 Brian J. Swetland
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions, and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions, and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef KERNEL_BOOT_BOOTDIR_H
#define KERNEL_BOOT_BOOTDIR_H

#define BOOTDIR_NAMELEN         32
#define BOOTDIR_MAX_ENTRIES     64
#define BOOTDIR_DIRECTORY       "SBBB/Directory"

typedef struct {
    char be_name[BOOTDIR_NAMELEN]; /* name of loaded object, zero terminated */
    int  be_offset;   /* offset of object relative to the start of boot_dir  */
    int  be_type;     /* object type designator                              */
    int  be_size;     /* size of loaded object (pages)                       */
    int  be_vsize;    /* size loaded object should occupy when mapped in     */
    int  be_extra0;
    int  be_extra1;
    int  be_extra2;
    int  be_extra3;
} boot_entry;

typedef struct {
    boot_entry bd_entry[BOOTDIR_MAX_ENTRIES];
} boot_dir;

/* void _start(uint32 mem, char *params, boot_dir *bd); */

#define BE_TYPE_NONE         0  /* empty entry                              */
#define BE_TYPE_DIRECTORY    1  /* directory (entry 0)                      */
#define BE_TYPE_BOOTSTRAP    2  /* bootstrap code object (entry 1)          */
#define BE_TYPE_CODE         3  /* executable code object                   */
#define BE_TYPE_DATA         4  /* raw data object                          */
#define BE_TYPE_ELF32        5  /* 32bit ELF object                         */

/* for BE_TYPE_CODE */
#define be_code_vaddr be_extra0 /* virtual address (rel offset 0)           */
#define be_code_ventr be_extra1 /* virtual entry point (rel offset 0)       */

#endif	/* KERNEL_BOOT_BOOTDIR_H */
