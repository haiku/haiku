/*
 * Copyright 2020, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _I2C_H_
#define _I2C_H_


#include <ACPI.h>
#include <device_manager.h>
#include <KernelExport.h>


typedef uint16 i2c_addr;
typedef enum {
	I2C_OP_READ = 0,
	I2C_OP_READ_STOP = 1,
	I2C_OP_WRITE = 2,
	I2C_OP_WRITE_STOP = 3,
	I2C_OP_READ_BLOCK = 5,
	I2C_OP_WRITE_BLOCK = 7,
} i2c_op;


#define IS_READ_OP(op)	(((op) & I2C_OP_WRITE) == 0)
#define IS_WRITE_OP(op)	(((op) & I2C_OP_WRITE) != 0)
#define IS_STOP_OP(op)	(((op) & 1) != 0)
#define IS_BLOCK_OP(op)	(((op) & 4) != 0)


// bus/device handle
typedef void* i2c_bus;
typedef void* i2c_device;


// Device node

// slave address (uint16)
#define I2C_DEVICE_SLAVE_ADDR_ITEM "i2c/slave_addr"

// node type
#define I2C_DEVICE_TYPE_NAME "i2c/device/v1"

// bus cookie, issued by i2c bus manager
typedef void* i2c_bus;
// device cookie, issued by i2c bus manager
typedef void* i2c_device;

// bus manager device interface for peripheral driver
typedef struct {
	driver_module_info info;

	status_t (*exec_command)(i2c_device cookie, i2c_op op,
		const void *cmdBuffer, size_t cmdLength, void* dataBuffer,
		size_t dataLength);
	status_t (*acquire_bus)(i2c_device cookie);
	void (*release_bus)(i2c_device cookie);
} i2c_device_interface;


#define I2C_DEVICE_MODULE_NAME "bus_managers/i2c/device/driver_v1"


// Bus node

#define I2C_BUS_PATH_ID_ITEM "i2c/path_id"

// node type
#define I2C_BUS_TYPE_NAME "i2c/bus"

// I2C bus node driver.
// This interface can be used by peripheral drivers to access the
// bus directly.
typedef struct {
	driver_module_info info;

	status_t (*exec_command)(i2c_bus cookie, i2c_op op, i2c_addr slaveAddress,
		const void *cmdBuffer, size_t cmdLength, void* dataBuffer,
		size_t dataLength);
	status_t (*acquire_bus)(i2c_bus cookie);
	void (*release_bus)(i2c_bus cookie);
} i2c_bus_interface;


// name of I2C bus node driver
#define I2C_BUS_MODULE_NAME "bus_managers/i2c/bus/driver_v1"



// Interface for Controllers

typedef struct {
	driver_module_info info;

	status_t (*register_device)(i2c_bus bus, i2c_addr slaveAddress,
		char* hid, char** cid, acpi_handle acpiHandle);
} i2c_for_controller_interface;

#define I2C_FOR_CONTROLLER_MODULE_NAME "bus_managers/i2c/controller/driver_v1"


// Controller Node

// node type
#define I2C_CONTROLLER_TYPE_NAME "bus/i2c/v1"
// controller name (required, string)
#define I2C_DESCRIPTION_CONTROLLER_NAME "controller_name"

typedef void *i2c_bus_cookie;
typedef void *i2c_intr_cookie;


// Bus manager interface used by I2C controller drivers.
typedef struct {
	driver_module_info info;

	void (*set_i2c_bus)(i2c_bus_cookie cookie, i2c_bus bus);

	status_t (*exec_command)(i2c_bus_cookie cookie, i2c_op op,
		i2c_addr slaveAddress, const void *cmdBuffer, size_t cmdLength,
		void* dataBuffer, size_t dataLength);

	status_t (*scan_bus)(i2c_bus_cookie cookie);

	status_t (*acquire_bus)(i2c_bus_cookie cookie);
	void (*release_bus)(i2c_bus_cookie cookie);

	status_t (*install_interrupt_handler)(i2c_bus_cookie cookie,
		i2c_intr_cookie intrCookie, interrupt_handler handler, void* data);

	status_t (*uninstall_interrupt_handler)(i2c_bus_cookie cookie,
		i2c_intr_cookie intrCookie);
} i2c_sim_interface;


// Devfs entry for raw bus access.
// used by i2c diag tool

enum {
	I2CEXEC = B_DEVICE_OP_CODES_END + 1,
};


typedef struct i2c_ioctl_exec
{
	i2c_op		op;
	i2c_addr	addr;
	const void*	cmdBuffer;
	size_t		cmdLength;
	void*		buffer;
	size_t		bufferLength;
} i2c_ioctl_exec;

#define I2C_EXEC_MAX_BUFFER_LENGTH  32

#endif	/* _I2C_H_ */
