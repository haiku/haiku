TARGETS = fsh tstfs

all : $(TARGETS)

zip:
	zip -y obfs-fskit-`date +%Y-%m-%d`.zip *.[ch]* makefile

#
# change the -O7 to -O3 if your compiler doesn't grok -O7
#
DEFINES = -DUSER=1 -DDEBUG=1
CFLAGS = -D_NO_INLINE_ASM -O0 -g -fno-exceptions -fno-rtti -I. -fcheck-memory-usage 
LDFLAGS = #-p

SUPPORT_OBJS = rootfs.o initfs.o kernel.o cache.o sl.o stub.o
MISC_OBJS    = sysdep.o hexdump.o argv.o

FS_OBJS = Volume.o BPlusTree.o Inode.o Index.o Query.o Journal.o \
	BlockAllocator.o kernel_interface.o Utility.o Debug.o BufferPool.o cpp.o


fsh : fsh.o $(FS_OBJS) $(SUPPORT_OBJS) $(MISC_OBJS)
	cc $(LDFLAGS) -o $@ fsh.o $(FS_OBJS) $(SUPPORT_OBJS) $(MISC_OBJS)

tstfs : tstfs.o $(FS_OBJS) $(SUPPORT_OBJS) $(MISC_OBJS)
	cc $(LDFLAGS) -o $@ tstfs.o $(FS_OBJS) $(SUPPORT_OBJS) $(MISC_OBJS)

makefs : makefs.o $(FS_OBJS) $(SUPPORT_OBJS) $(MISC_OBJS)
	cc $(LDFLAGS) -o $@ makefs.o $(FS_OBJS) $(SUPPORT_OBJS) $(MISC_OBJS)


.c.o:
	$(CC) -c $(DEFINES) $(CFLAGS) -o $@ $<

.cpp.o:
	$(CC) -c $(DEFINES) $(CFLAGS) -o $@ $<


#makefs.o : makefs.c bfs.h
fsh.o    : fsh.c bfs.h
tstfs.o  : tstfs.c bfs.h


# mount.o     : mount.c myfs.h
# journal.o   : journal.c myfs.h
# bitmap.o    : bitmap.c myfs.h
# inode.o     : inode.c myfs.h
# dstream.o   : dstream.c myfs.h
# dir.o       : dir.c myfs.h
# file.o      : file.c myfs.h 
# bitvector.o : bitvector.c bitvector.h 
# util.o      : util.c myfs.h
# 
bfs.h : compat.h cache.h lock.h fsproto.h

sysdep.o : sysdep.c compat.h 
kernel.o : kernel.c compat.h fsproto.h kprotos.h
rootfs.o : compat.h fsproto.h
initfs.o : initfs.c compat.h fsproto.h
sl.o     : sl.c skiplist.h
cache.o  : cache.c cache.h compat.h
stub.o   : stub.c compat.h

clean:
	rm -f *.o $(TARGETS)
