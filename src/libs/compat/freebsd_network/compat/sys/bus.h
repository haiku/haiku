/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_BUS_H_
#define _FBSD_COMPAT_SYS_BUS_H_


#include <sys/systm.h>
#include <sys/sysctl.h>

#include <sys/haiku-module.h>

#include <sys/_bus_dma.h>


// TODO per platform, these are 32-bit
typedef uint32_t bus_addr_t;
typedef uint32_t bus_size_t;

#define BUS_SPACE_MAXADDR_32BIT		0xffffffff
#define BUS_SPACE_MAXADDR			0xffffffff

#define BUS_SPACE_MAXSIZE_32BIT		0xffffffff
#define BUS_SPACE_MAXSIZE			0xffffffff

typedef int	bus_space_tag_t;
typedef unsigned int bus_space_handle_t;

uint8_t bus_space_read_1(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset);
uint16_t bus_space_read_2(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset);
uint32_t bus_space_read_4(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset);
void bus_space_write_1(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, uint8_t value);
void bus_space_write_2(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, uint16_t value);
void bus_space_write_4(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, uint32_t value);

// oh you glorious world of macros
#define bus_space_write_stream_4(t, h, o, v) \
	bus_space_write_4((t), (h), (o), (v))
#define bus_read_1(r, o) \
	bus_space_read_1((r)->r_bustag, (r)->r_bushandle, (o))
#define bus_read_2(r, o) \
	bus_space_read_2((r)->r_bustag, (r)->r_bushandle, (o))
#define bus_read_4(r, o) \
	bus_space_read_4((r)->r_bustag, (r)->r_bushandle, (o))
#define bus_write_1(r, o, v) \
	bus_space_write_1((r)->r_bustag, (r)->r_bushandle, (o), (v))
#define bus_write_2(r, o, v) \
	bus_space_write_2((r)->r_bustag, (r)->r_bushandle, (o), (v))
#define bus_write_4(r, o, v) \
	bus_space_write_4((r)->r_bustag, (r)->r_bushandle, (o), (v))


#define BUS_SPACE_BARRIER_READ		1
#define BUS_SPACE_BARRIER_WRITE		2

static inline void
bus_space_barrier(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, bus_size_t len, int flags)
{
	if (flags & BUS_SPACE_BARRIER_READ)
		__asm__ __volatile__ ("lock; addl $0,0(%%esp)" : : : "memory");
	else
		__asm__ __volatile__ ("" : : : "memory");
}

#define bus_barrier(r, o, l, f) \
	bus_space_barrier((r)->r_bustag, (r)->r_bushandle, (o), (l), (f))

struct resource;

struct resource_spec {
	int	type;
	int	rid;
	int	flags;
};

enum intr_type {
	INTR_TYPE_NET	= 4,
	INTR_FAST		= 128,
	INTR_MPSAFE		= 512,
};

#define	FILTER_STRAY			B_UNHANDLED_INTERRUPT
#define	FILTER_HANDLED			B_HANDLED_INTERRUPT
#define	FILTER_SCHEDULE_THREAD	B_INVOKE_SCHEDULER

/* Note that we reversed the original order, so whenever actual (negative)
   numbers are used in a driver, we have to change it. */
#define BUS_PROBE_SPECIFIC		0
#define BUS_PROBE_LOW_PRIORITY	10
#define BUS_PROBE_DEFAULT		20
#define BUS_PROBE_GENERIC		100

int bus_generic_detach(device_t dev);
int bus_generic_suspend(device_t dev);
int bus_generic_resume(device_t dev);
void bus_generic_shutdown(device_t dev);

typedef int (*driver_filter_t)(void *);
typedef void (*driver_intr_t)(void *);


int resource_int_value(const char *name, int unit, const char *resname,
	int *result);

struct resource *bus_alloc_resource(device_t dev, int type, int *rid,
	unsigned long start, unsigned long end, unsigned long count, uint32 flags);
int bus_release_resource(device_t dev, int type, int rid, struct resource *r);
int bus_alloc_resources(device_t dev, struct resource_spec *resourceSpec,
	struct resource **resources);
void bus_release_resources(device_t dev,
	const struct resource_spec *resourceSpec, struct resource **resources);

static inline struct resource *
bus_alloc_resource_any(device_t dev, int type, int *rid, uint32 flags)
{
	return bus_alloc_resource(dev, type, rid, 0, ~0, 1, flags);
}

bus_dma_tag_t bus_get_dma_tag(device_t dev);

int bus_setup_intr(device_t dev, struct resource *r, int flags,
	driver_filter_t filter, driver_intr_t handler, void *arg, void **_cookie);
int bus_teardown_intr(device_t dev, struct resource *r, void *cookie);

const char *device_get_name(device_t dev);
const char *device_get_nameunit(device_t dev);
int device_get_unit(device_t dev);
void *device_get_softc(device_t dev);
int device_printf(device_t dev, const char *, ...) __printflike(2, 3);
void device_set_desc(device_t dev, const char *desc);
void device_set_desc_copy(device_t dev, const char *desc);
const char *device_get_desc(device_t dev);
device_t device_get_parent(device_t dev);

void device_set_ivars(device_t dev, void *);
void *device_get_ivars(device_t dev);

device_t device_add_child(device_t dev, const char *name, int unit);
int device_delete_child(device_t dev, device_t child);
int device_is_attached(device_t dev);
int device_attach(device_t dev);
int device_detach(device_t dev);
int bus_generic_print_child(device_t dev, device_t child);
void bus_generic_driver_added(device_t dev, driver_t *driver);
int bus_generic_attach(device_t dev);
int device_set_driver(device_t dev, driver_t *driver);
int device_is_alive(device_t dev);

static inline struct sysctl_ctx_list *
device_get_sysctl_ctx(device_t dev)
{
	return NULL;
}

static inline void *
device_get_sysctl_tree(device_t dev)
{
	return NULL;
}

#include <sys/bus_dma.h>

#endif	/* _FBSD_COMPAT_SYS_BUS_H_ */
