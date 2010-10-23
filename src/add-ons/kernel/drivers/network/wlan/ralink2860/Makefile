
SRCS = 	device_if.h \
		bus_if.h \
		pci_if.h \
		rt2860_io.c \
		rt2860_read_eeprom.c \
		rt2860_led.c \
		rt2860_rf.c \
		rt2860_amrr.c \
		rt2860.c

KMOD = rt2860

DEBUG_FLAGS = -g
WERROR =
CFLAGS += -DRT2860_DEBUG

.include <bsd.kmod.mk>
