ifeq ($(WHAT)x, x)

###
include $(BUILDHOME)/etc/makefile-engine
###

LIST = \
	unzip \
	funzip \
	unzipsfx

install: FORCE
	for WHAT in $(LIST); do \
		$(MAKE) $(PARALLELMFLAGS) WHAT=$$WHAT $@;  \
	done
	ln -sf unzip $(INSTALL_BIN)/zipinfo
	install -m 0644 unix/zipgrep $(INSTALL_BIN)

clean:	 FORCE
	-rm -rf $(OBJ_DIR)

.DEFAULT: FORCE
	for WHAT in $(LIST); do \
		$(MAKE) $(PARALLELMFLAGS) WHAT=$$WHAT $@; \
	done

FORCE:

else

SHELL = /bin/sh

###
include $(BUILDHOME)/etc/makefile-engine
###

TARGET		= $(OBJ_DIR)/$(WHAT)

APP_VERSION		= -app 5 4 0 f 0 -short 5.4 -long "5.4 "`echo -n -e '\302\251'`" 1998 Info-ZIP et. al."

LDFLAGS += -lbe

VPATH		= .:beos

ifeq ($(WHAT), unzip)
SRCS		= \
				unzip.c \
				crc32.c \
				crctab.c \
				crypt.c \
				envargs.c \
				explode.c \
				extract.c \
				fileio.c \
				globals.c \
				inflate.c \
				list.c \
				match.c \
				process.c \
				ttyio.c \
				unreduce.c \
				unshrink.c \
				zipinfo.c  \
				beos.c    \
				beosmain.c
endif

ifeq ($(WHAT), unzipsfx)
SRCS		= \
				unzipsfx.c \
				crc32.c \
				crctab.c \
				crypt.c \
				extract_.c \
				fileio.c \
				globals.c \
				inflate.c \
				match.c \
				process_.c \
				ttyio.c \
				beos_.c    \
				beosmain.c
endif

ifeq ($(WHAT), funzip)
SRCS		= \
				funzip.c \
				crc32.c \
				crypt_.c \
				globals_.c \
				inflate_.c \
				ttyio_.c
endif

OBJS		= $(SRCS_LIST_TO_OBJS)

INCLUDES 	+= -I. -Ibeos
CFLAGS		+= -DPASSWD_FROM_STDIN

$(TARGET):	$(OBJ_DIR) $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS)
	$(SETVERSION) $@ $(SYSTEM_VERSION) $(APP_VERSION)

$(OBJ_DIR)/crypt_.o:	crypt.c $(OBJ_DIR)
	cp -f crypt.c $(OBJ_DIR)/crypt_.c
	$(CC) -c $(INCLUDES) $(CFLAGS) -DFUNZIP $(OBJ_DIR)/crypt_.c -o $(OBJ_DIR)/crypt_.o

$(OBJ_DIR)/extract_.o:	extract.c $(OBJ_DIR)
	cp -f extract.c $(OBJ_DIR)/extract_.c
	$(CC) -c $(INCLUDES) $(CFLAGS) -DSFX $(OBJ_DIR)/extract_.c -o $(OBJ_DIR)/extract_.o

$(OBJ_DIR)/globals_.o:	globals.c $(OBJ_DIR)
	cp -f globals.c $(OBJ_DIR)/globals_.c
	$(CC) -c $(INCLUDES) $(CFLAGS) -DFUNZIP $(OBJ_DIR)/globals_.c -o $(OBJ_DIR)/globals_.o

$(OBJ_DIR)/inflate_.o:	inflate.c $(OBJ_DIR)
	cp -f inflate.c $(OBJ_DIR)/inflate_.c
	$(CC) -c $(INCLUDES) $(CFLAGS) -DFUNZIP $(OBJ_DIR)/inflate_.c -o $(OBJ_DIR)/inflate_.o

$(OBJ_DIR)/ttyio_.o:	ttyio.c $(OBJ_DIR)
	cp -f ttyio.c $(OBJ_DIR)/ttyio_.c
	$(CC) -c $(INCLUDES) $(CFLAGS) -DFUNZIP $(OBJ_DIR)/ttyio_.c -o $(OBJ_DIR)/ttyio_.o

$(OBJ_DIR)/process_.o:	process.c $(OBJ_DIR)
	cp -f process.c $(OBJ_DIR)/process_.c
	$(CC) -c $(INCLUDES) $(CFLAGS) -DSFX $(OBJ_DIR)/process_.c -o $(OBJ_DIR)/process_.o

$(OBJ_DIR)/beos_.o:		beos/beos.c $(OBJ_DIR)
	cp -f beos/beos.c $(OBJ_DIR)/beos_.c
	$(CC) -c $(INCLUDES) $(CFLAGS) -DSFX $(OBJ_DIR)/beos_.c -o $(OBJ_DIR)/beos_.o

$(OBJ_DIR)/beosmain.o:	beos/beosmain.cpp $(OBJ_DIR)
	$(CC) -c $(INCLUDES) $(CFLAGS) beos/beosmain.cpp -o $(OBJ_DIR)/beosmain.o


$(OBJ_DIR)/unzipsfx.o:	unzip.c $(OBJ_DIR)
	cp -f unzip.c $(OBJ_DIR)/unzipsfx.c
	$(CC) -c $(INCLUDES) $(CFLAGS) -DSFX $(OBJ_DIR)/unzipsfx.c -o $(OBJ_DIR)/unzipsfx.o

###
###

endif
