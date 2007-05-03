#ifndef _FBSD_COMPAT_SYS_BUS_H_
#define _FBSD_COMPAT_SYS_BUS_H_

#include <arch/x86/arch_cpu.h>

#include <sys/systm.h>

// TODO per platform, these are x86
typedef uint32_t	bus_addr_t;
typedef uint32_t	bus_size_t;

typedef int			bus_space_tag_t;
typedef unsigned int	bus_space_handle_t;

#define I386_BUS_SPACE_IO			0
#define I386_BUS_SPACE_MEM			1

#define BUS_SPACE_MAXADDR_32BIT		0xffffffff
#define BUS_SPACE_MAXADDR			0xffffffff

#define BUS_SPACE_MAXSIZE_32BIT		0xffffffff
#define BUS_SPACE_MAXSIZE			0xffffffff

static inline uint8_t
bus_space_read_1(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset)
{
	if (tag == I386_BUS_SPACE_IO)
		return in8(handle + offset);
	return *(volatile uint8_t *)(handle + offset);
}


static inline void
bus_space_write_1(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, uint8_t value)
{
	if (tag == I386_BUS_SPACE_IO)
		out8(handle + offset, value);
	else
		*(volatile uint8_t *)(handle + offset) = value;
}


static inline uint16_t
bus_space_read_2(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset)
{
	if (tag == I386_BUS_SPACE_IO)
		return in16(handle + offset);
	return *(volatile uint16_t *)(handle + offset);
}


static inline void
bus_space_write_2(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, uint16_t value)
{
	if (tag == I386_BUS_SPACE_IO)
		out16(handle + offset, value);
	else
		*(volatile uint16_t *)(handle + offset) = value;
}


static inline uint32_t
bus_space_read_4(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset)
{
	if (tag == I386_BUS_SPACE_IO)
		return in32(handle + offset);
	return *(volatile uint32_t *)(handle + offset);
}


static inline void
bus_space_write_4(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, uint32_t value)
{
	if (tag == I386_BUS_SPACE_IO)
		out32(handle + offset, value);
	else
		*(volatile uint32_t *)(handle + offset) = value;
}


typedef struct device *device_t;
typedef struct devclass *devclass_t;

typedef int (*device_method_signature_t)(device_t dev);

struct device_method {
	const char *name;
	device_method_signature_t method;
};

typedef struct device_method device_method_t;

#define DEVMETHOD(name, func)	{ #name, (device_method_signature_t)func }

typedef struct {
	const char *name;
	device_method_t *methods;
	size_t size;
} driver_t;

enum intr_type {
	INTR_TYPE_NET	= 4,
	INTR_MPSAFE		= 512,
};

int bus_generic_detach(device_t dev);

typedef void (*driver_intr_t)(void *);

struct resource;

int resource_int_value(const char *name, int unit, const char *resname,
	int *result);

struct resource *bus_alloc_resource(device_t dev, int type, int *rid,
	unsigned long start, unsigned long end, unsigned long count, uint32 flags);
int bus_release_resource(device_t dev, int type, int rid, struct resource *r);

static inline struct resource *
bus_alloc_resource_any(device_t dev, int type, int *rid, uint32 flags)
{
	return bus_alloc_resource(dev, type, rid, 0, ~0, 1, flags);
}

int bus_setup_intr(device_t dev, struct resource *r, int flags,
	driver_intr_t handler, void *arg, void **cookiep);
int bus_teardown_intr(device_t dev, struct resource *r, void *cookie);

#define BUS_PROBE_DEFAULT		-20

#define DRIVER_MODULE(name, busname, driver, devclass, evh, arg)

const char *device_get_name(device_t dev);
const char *device_get_nameunit(device_t dev);
int device_get_unit(device_t dev);
void *device_get_softc(device_t dev);
int device_printf(device_t dev, const char *, ...) __printflike(2, 3);
void device_set_desc(device_t dev, const char *desc);

device_t device_add_child(device_t dev, const char *name, int unit);
int device_delete_child(device_t dev, device_t child);

#include <sys/bus_dma.h>

#endif
