# Target: Intel 586 running Haiku

TDEPFILES= i386-tdep.o i386-haiku-tdep.o haiku-tdep.o i387-tdep.o \
	solib.o solib-haiku.o symfile-mem.o
DEPRECATED_TM_FILE= tm-haiku.h
