/*	$OpenBSD: bootxx.c,v 1.1 1997/09/17 10:46:16 downsj Exp $	*/
/*	$NetBSD: bootxx.c,v 1.2 1997/09/14 19:28:17 pk Exp $	*/

/*
 * Copyright (c) 1994 Paul Kranenburg
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Paul Kranenburg.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/time.h>

#include <stand.h>

#include <promdev.h>

int debug;
int netif_debug;

/*
 * Boot device is derived from ROM provided information.
 */
const char		progname[] = "bootxx";
struct open_file	io;

/*
 * The contents of the block_* variables below is set by installboot(8)
 * to hold the the filesystem data of the second-stage boot program
 * (typically `/boot'): filesystem block size, # of filesystem blocks and
 * the block numbers themselves.
 */
#define MAXBLOCKNUM	256	/* enough for a 2MB boot program (bs 8K) */
int32_t			block_size = 0;
int32_t			block_count = MAXBLOCKNUM;
daddr_t			block_table[MAXBLOCKNUM] = { 0 };

int 			memory_node = 0;

void	loadboot __P((struct open_file *, caddr_t));

void scan_node(int node, char *buf, int recurse, int level)
{
	char *ptr;
	int node1; 

	do {
//		printf("%dnode 0x%x\n", level, node);
		ptr = NULL;
		for(ptr = promvec->pv_nodeops->no_nextprop(node, NULL); 
			ptr != NULL && ptr[0] != '\0'; 
			ptr = promvec->pv_nodeops->no_nextprop(node, ptr)) {

//			printf("%d\tproperty '%s'\n", level, ptr);

			if(!strcmp(ptr, "name")) {
				promvec->pv_nodeops->no_getprop(node, ptr, buf);
//				printf("%d\t\tname = '%s'\n", level, buf);
				if(!strcmp(buf, "memory")) {
					memory_node = node;
				}
			}

		}
		node1 = promvec->pv_nodeops->no_child(node);
//		printf("%d\tchild = 0x%x\n", level, node1);
		if(node1 != 0 && recurse != 0) {
			scan_node(node1, buf, recurse, level+1);
		}
		node = promvec->pv_nodeops->no_nextnode(node);
	} while(node != 0);
}

void scan_nodes()
{
	char buf[64];

	// Walk through the nodes
	{
		int node = 0; 

		node = promvec->pv_nodeops->no_nextnode(node);
		scan_node(node, buf, 1, 0);
	}
}

/*
 * Internal form of getprop().  Returns the actual length.
 */
int
getprop(node, name, buf, bufsiz)
	int node;
	char *name;
	void *buf;
	register int bufsiz;
{
#if defined(SUN4C) || defined(SUN4M)
	register struct nodeops *no;
	register int len;
#endif

#if defined(SUN4)
	if (CPU_ISSUN4) {
		printf("WARNING: getprop not valid on sun4! %s\n", name);
		return (0);
	}
#endif

#if defined(SUN4C) || defined(SUN4M)
	no = promvec->pv_nodeops;
	len = no->no_proplen(node, name);
	if (len > bufsiz) {
		printf("node 0x%x property %s length %d > %d\n",
		    node, name, len, bufsiz);
#ifdef DEBUG
		panic("getprop");
#else
		return (0);
#endif
	}
	no->no_getprop(node, name, buf);
	return (len);
#endif
}

int
main()
{
	char	*dummy;
	size_t	n;
	register void (*entry)__P((caddr_t)) = (void (*)__P((caddr_t)))LOADADDR;

	prom_init();

	printf("Welcome to second stage bootloader!\n");

	printf("cputyp = %d\n", cputyp);
	printf("nbpg = %d\n", nbpg);
	printf("pgofset = %d\n", pgofset);
	printf("pgshift = %d\n", pgshift);
	printf("promvec = 0x%x\n", (unsigned int)promvec);

	printf("Scanning nodes...");
	scan_nodes();
	printf("done\n");
	printf("memory_node = 0x%x\n", memory_node);

	// Look at how memory is laid out
	{
		struct openprom_addr addr[64];
		int len;
		int i;

		len = getprop(memory_node, "reg", &addr, sizeof(addr)) /
			sizeof(struct openprom_addr);
		printf("retrieved physical memory layout struct. size %d:\n", len);

		for(i=0; i<len; i++) {
			printf("\tstart addr 0x%x, len 0x%x\n", 
				addr[i].oa_base, addr[i].oa_size);
		}

		len = getprop(memory_node, "available", &addr, sizeof(addr)) /
			sizeof(struct openprom_addr);
		printf("retrieved available physical memory layout struct. size %d:\n", len);

		for(i=0; i<len; i++) {
			printf("\tstart addr 0x%x, len 0x%x\n", 
				addr[i].oa_base, addr[i].oa_size);
		}
	}

/*
	io.f_flags = F_RAW;
	if (devopen(&io, 0, &dummy)) {
		panic("%s: can't open device", progname);
	}
	(void)loadboot(&io, LOADADDR);
	(io.f_dev->dv_close)(&io);
	(*entry)(cputyp == CPU_SUN4 ? LOADADDR : (caddr_t)promvec);
*/
	_rtt();
}

void
loadboot(f, addr)
	register struct open_file	*f;
	register char			*addr;
{
	return;
}

#if 0
void
loadboot(f, addr)
	register struct open_file	*f;
	register char			*addr;
{
	register int	i;
	register char	*buf;
	size_t		n;
	daddr_t		blk;

	/*
	 * Allocate a buffer that we can map into DVMA space; only
	 * needed for sun4 architecture, but use it for all machines
	 * to keep code size down as much as possible.
	 */
	buf = alloc(block_size);
	if (buf == NULL)
		panic("%s: alloc failed", progname);

	for (i = 0; i < block_count; i++) {
		if ((blk = block_table[i]) == 0)
			panic("%s: block table corrupt", progname);

#ifdef DEBUG
		printf("%s: block # %d = %d\n", progname, i, blk);
#endif
		if ((f->f_dev->dv_strategy)(f->f_devdata, F_READ,
					    blk, block_size, buf, &n)) {
			panic("%s: read failure", progname);
		}
		bcopy(buf, addr, block_size);
		if (n != block_size)
			panic("%s: short read", progname);
		if (i == 0) {
			register int m = N_GETMAGIC(*(struct exec *)addr);
			if (m == ZMAGIC || m == NMAGIC || m == OMAGIC) {
				/* Move exec header out of the way */
				bcopy(addr, addr - sizeof(struct exec), n);
				addr -= sizeof(struct exec);
			}
		}
		addr += n;
	}

}
#endif

