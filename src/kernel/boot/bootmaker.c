/* 
** Some Portions Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/*
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
#include "../../../../../headers/obos/private/kernel/bootdir.h"

//#include "sparcbootblock.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef sparc
#define xBIG_ENDIAN 1
#endif
#ifdef i386
#define xLITTLE_ENDIAN 1
#endif
#ifdef __ppc__
#define xBIG_ENDIAN 1
#endif

#define SWAP32(x) \
	((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24))

#if xBIG_ENDIAN
#define HOST_TO_BENDIAN32(x) (x)
#define BENDIAN_TO_HOST32(x) (x)
#define HOST_TO_LENDIAN32(x) SWAP32(x)
#define LENDIAN_TO_HOST32(x) SWAP32(x)
#endif
#if xLITTLE_ENDIAN
#define HOST_TO_BENDIAN32(x) SWAP32(x)
#define BENDIAN_TO_HOST32(x) SWAP32(x)
#define HOST_TO_LENDIAN32(x) (x)
#define LENDIAN_TO_HOST32(x) (x)
#endif

#if !xBIG_ENDIAN && !xLITTLE_ENDIAN
#error not sure which endian the host processor is, please edit bootmaker.c
#endif

// ELF stuff
#define ELF_MAGIC "\x7f""ELF"
#define EI_MAG0	0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_PAD 7
#define EI_NIDENT 16

#define ELFCLASS32 1
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

// XXX not safe across all build architectures
typedef unsigned int Elf32_Addr;
typedef unsigned short Elf32_Half;
typedef unsigned int Elf32_Off;
typedef int Elf32_Sword;
typedef unsigned int Elf32_Word;

struct Elf32_Ehdr {
	unsigned char	e_ident[EI_NIDENT];
	Elf32_Half		e_type;
	Elf32_Half		e_machine;
	Elf32_Word		e_version;
	Elf32_Addr		e_entry;
	Elf32_Off		e_phoff;
	Elf32_Off		e_shoff;
	Elf32_Word		e_flags;
	Elf32_Half		e_ehsize;
	Elf32_Half		e_phentsize;
	Elf32_Half		e_phnum;
	Elf32_Half		e_shentsize;
	Elf32_Half		e_shnum;
	Elf32_Half		e_shstrndx;
};

struct Elf32_Phdr {
	Elf32_Word		p_type;
	Elf32_Off		p_offset;
	Elf32_Addr		p_vaddr;
	Elf32_Addr		p_paddr;
	Elf32_Word		p_filesz;
	Elf32_Word		p_memsz;
	Elf32_Word		p_flags;
	Elf32_Word		p_align;
};

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define LE 0
#define BE 1

static int target_endian = LE;

#define fix(x) ((target_endian == BE) ? HOST_TO_BENDIAN32(x) : HOST_TO_LENDIAN32(x))

static int make_sparcboot = 0;
static int strip_debug = 0;
static char *strip_binary = "strip";

void die(char *s, char *a)
{
    fprintf(stderr,"error: ");
    fprintf(stderr,s,a);
    fprintf(stderr,"\n");
    exit(1);
}

void *loadfile(char *file, int *size)
{
    int fd;
    char *data;
    struct stat info;

    if((fd = open(file,O_BINARY|O_RDONLY)) != -1){
        if(fstat(fd,&info)){
            close(fd);
            *size = 0;
            return NULL;
        }
        data = (char *) malloc(info.st_size);
        if(read(fd, data, info.st_size) != info.st_size) {
            close(fd);
            *size = 0;
            return NULL;
        }
        close(fd);
        *size = info.st_size;
        return data;
    }
    *size = 0;
    return NULL;
}

void *loadstripfile(char *file, int *size)
{
    char temp[256];
    char cmd[4096];
    void *retval;


    if(strip_debug) {
        strcpy(temp, "/tmp/mkboot.XXXXXXXX");
        mktemp(temp);
        sprintf(cmd, "cp %s %s; %s %s", file, temp, strip_binary, temp);
        system(cmd);

        retval = loadfile(temp, size);

        unlink(temp);
    } else {
        retval = loadfile(file, size);
    }

    return retval;
}

// write a boot block to the head of the dir.
// note: the first 0x20 bytes are removed by the sparc prom
// which makes the whole file off by 0x20 bytes
/*
int writesparcbootblock(int fd, unsigned int blocks)
{
	unsigned char bb[0x200+0x20];

	memset(bb, 0, sizeof(bb));
	memcpy(bb, sparcbootblock, sizeof(sparcbootblock));

	return write(fd, bb, sizeof(bb));
}
*/

typedef struct _nvpair 
{
    struct _nvpair *next;
    char *name;
    char *value;
} nvpair;


typedef struct _section
{
    struct _section *next;
    char *name;
    struct _nvpair *firstnv;
} section;

void print_sections(section *first)
{
    nvpair *p;
    
    while(first){
        printf("\n[%s]\n",first->name);
        for(p = first->firstnv; p; p = p->next){
            printf("%s=%s\n",p->name,p->value);
        }
        first = first->next;
    }
}

#define stNEWLINE 0
#define stSKIPLINE 1
#define stHEADER 2
#define stLHS 3
#define stRHS 4

section *first = NULL;
section *last = NULL;

section *load_ini(char *file)
{
    char *data,*end;
    int size;
    int state = stNEWLINE;
	section *cur;
	
    char *lhs,*rhs;
    
    if(!(data = loadfile(file,&size))){
        return NULL;
    }
    end = data+size;
    
    while(data < end){
        switch(state){
        case stSKIPLINE:
            if(*data == '\n' || *data == '\r'){
                state = stNEWLINE;
            }
            data++;
            break;
            
        case stNEWLINE:
            if(*data == '\n' || *data == '\r'){
                data++;
                break;
            }
            if(*data == '['){
                lhs = data+1;
                state = stHEADER;
                data++;
                break;
            }
            if(*data == '#' || *data <= ' '){
                state = stSKIPLINE;
                data++;
                break;
            }
            lhs = data;
            data++;
            state = stLHS;
            break;
        case stHEADER:        
            if(*data == ']'){                
                cur = (section *) malloc(sizeof(section));
                cur->name = lhs;
                cur->firstnv = NULL;
                cur->next = NULL;
                if(last){
                    last->next = cur;
                    last = cur;
                } else {
                    last = first = cur;
                }
                *data = 0;
                state = stSKIPLINE;
            }
            data++;
            break;
        case stLHS:
            if(*data == '\n' || *data == '\r'){
                state = stNEWLINE;
            }
            if(*data == '='){
                *data = 0;
                rhs = data+1;
                state = stRHS;
            }
            data++;
            continue;
        case stRHS:
            if(*data == '\n' || *data == '\r'){
                nvpair *p = (nvpair *) malloc(sizeof(nvpair));
                p->name = lhs;
                p->value = rhs;
                *data = 0;
                p->next = cur->firstnv;
                cur->firstnv = p;
                state = stNEWLINE;
            }
            data++;
            break;
        }
    }
    return first;
    
}


char *getval(section *s, char *name)
{
    nvpair *p;
    for(p = s->firstnv; p; p = p->next){
        if(!strcmp(p->name,name)) return p->value;
    }
    return NULL;
}

char *getvaldef(section *s, char *name, char *def)
{
    nvpair *p;
    for(p = s->firstnv; p; p = p->next){
        if(!strcmp(p->name,name)) return p->value;
    }
    return def;
}

Elf32_Addr elf_find_entry(void *buf, int size)
{
	struct Elf32_Ehdr *header;
	struct Elf32_Phdr *pheader;
	char *cbuf = buf;
	int byte_swap;
	int index;

#define SWAPIT(x) ((byte_swap) ? SWAP32(x) : (x))

	if(memcmp(cbuf, ELF_MAGIC, sizeof(ELF_MAGIC)-1) != 0)
		return 0;

	if(cbuf[EI_CLASS] != ELFCLASS32)
		return 0;

	byte_swap = 0;
#if xBIG_ENDIAN		
	if(cbuf[EI_DATA] == ELFDATA2LSB) {
		byte_swap = 1;
	}
#else
	if(cbuf[EI_DATA] == ELFDATA2MSB) {
		byte_swap = 1;
	}
#endif
	
	header = (struct Elf32_Ehdr *)cbuf;
	pheader = (struct Elf32_Phdr *)&cbuf[SWAPIT(header->e_phoff)];

	// XXX only looking at the first program header. Should be ok
	return SWAPIT(pheader->p_offset);
}
#undef SWAPIT

#define centry bdir.bd_entry[c]
void makeboot(section *s, char *outfile)
{
    int fd;
    void *rawdata[64];
    int rawsize[64];
    char fill[4096];
    boot_dir bdir;
    int i,c;
    int nextpage = 1; /* page rel offset of next loaded object */

    memset(fill,0,4096);
    
    memset(&bdir, 0, 4096);
    for(i=0;i<64;i++){
        rawdata[i] = NULL;
        rawsize[i] = 0;
    }

    c = 1;

    bdir.bd_entry[0].be_type = fix(BE_TYPE_DIRECTORY);
    bdir.bd_entry[0].be_size = fix(1);
    bdir.bd_entry[0].be_vsize = fix(1);
    rawdata[0] = (void *) &bdir;
    rawsize[0] = 4096;
    
    strcpy(bdir.bd_entry[0].be_name,"SBBB/Directory");

    while(s){
        char *type = getvaldef(s,"type","NONE");
        char *file = getval(s,"file");
        int vsize;
        int size;
        struct stat statbuf;
        
        if(!type) die("section %s has no type",s->name);

        strncpy(centry.be_name,s->name,32);
        centry.be_name[31] = 0;

        if(!file) die("section %s has no file",s->name);
        rawdata[c]= ((strcmp(type, "elf32")==0)?loadstripfile:loadfile)(file,&rawsize[c]);
        if(!rawdata[c])
           die("cannot load \"%s\"",file);

        if(stat(file,&statbuf))
            die("cannot stat \"%s\"",file);
        vsize = statbuf.st_size;
        
        centry.be_size = rawsize[c] / 4096 + (rawsize[c] % 4096 ? 1 : 0);
        centry.be_vsize = 
            (vsize < centry.be_size) ? centry.be_size : vsize;

        centry.be_offset = nextpage;
        nextpage += centry.be_size;

        centry.be_size = fix(centry.be_size);
        centry.be_vsize = fix(centry.be_vsize);
        centry.be_offset = fix(centry.be_offset);
        
        if(!strcmp(type,"boot")){
            centry.be_type = fix(BE_TYPE_BOOTSTRAP);
            centry.be_code_vaddr = fix(atoi(getvaldef(s,"vaddr","0")));
            centry.be_code_ventr = fix(atoi(getvaldef(s,"ventry","0")));            
        }
        if(!strcmp(type,"code")){
            centry.be_type = fix(BE_TYPE_CODE);
            centry.be_code_vaddr = fix(atoi(getvaldef(s,"vaddr","0")));
            centry.be_code_ventr = fix(atoi(getvaldef(s,"ventry","0"))); 
        }
        if(!strcmp(type,"data")){
            centry.be_type = fix(BE_TYPE_DATA);
        }
        if(!strcmp(type,"elf32")){
            centry.be_type = fix(BE_TYPE_ELF32);
            centry.be_code_vaddr = 0;
            centry.be_code_ventr = fix(elf_find_entry(rawdata[c], rawsize[c]));
        }

        if(centry.be_type == BE_TYPE_NONE){
            die("unrecognized section type \"%s\"",type);
        }

        c++;
        s = s->next;
        
        if(c==64) die("too many sections (>63)",NULL);
    }

    if((fd = open(outfile, O_BINARY|O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0) {
        die("cannot write to \"%s\"",outfile);
    }

/* XXX - Hope this isn't needed :(
    if(make_sparcboot) {
        writesparcbootblock(fd, nextpage+1);
    }
*/
    for(i=0;i<c;i++){
        write(fd, rawdata[i], rawsize[i]);
        if(rawsize[i]%4096) 
            write(fd, fill, 4096 - (rawsize[i]%4096));
    }
    close(fd);
}

int main(int argc, char **argv)
{
	char *file = NULL;
    section *s;
    
    if(argc < 2){
usage:
        fprintf(stderr,"usage: %s [--littleendian (default)] [--bigendian ] [ --strip-binary <binary ] [ --strip-debug] [ --sparc | -s ] [ <inifile> ... ] -o <bootfile>\n",argv[0]);
        return 1;
    }

	argc--;
	argv++;
	
	while(argc){
		if(!strcmp(*argv,"--sparc")) {
			make_sparcboot = 1;
		} else if(!strcmp(*argv, "--bigendian")) {
			target_endian = BE;
		} else if(!strcmp(*argv,"-o")) {
			argc--;
			argv++;
			if(argc) {
				file = *argv;
			} else {
				goto usage;
			}
		} else if(!strcmp(*argv, "--strip-binary")) {
			argc--;
			argv++;
			if(argc) {
				strip_binary = *argv;
			} else {
				goto usage;
			}
		} else if(!strcmp(*argv, "--strip-debug")) {
			strip_debug = 1;
		} else {
			if(load_ini(*argv) == NULL) {
				fprintf(stderr,"warning: cannot load '%s'\n",*argv);
			}
		}
		argc--;
		argv++;
	}
	
	
    if((argc > 3) && !strcmp(argv[3],"-sparc")){
        make_sparcboot = 1;
    }

	if(!file){
		fprintf(stderr,"error: no output specified\n");
		goto usage;
	}
	
	if(!first){
		fprintf(stderr,"error: no data to write?!\n");
		goto usage;
	}
	
	makeboot(first,file);
    
    return 0;    
}

