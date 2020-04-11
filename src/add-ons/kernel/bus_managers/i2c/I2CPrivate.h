/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef I2C_PRIVATE_H
#define I2C_PRIVATE_H


#include <new>
#include <stdio.h>
#include <string.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <i2c.h>


//#define I2C_TRACE
#ifdef I2C_TRACE
#	define TRACE(x...)		dprintf("\33[33mi2c:\33[0m " x)
#else
#	define TRACE(x...)
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[33mi2c:\33[0m " x)
#define ERROR(x...)			TRACE_ALWAYS(x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define I2C_SIM_MODULE_NAME		"bus_managers/i2c/sim/driver_v1"

#define I2C_PATHID_GENERATOR "i2c/path_id"
#define I2C_BUS_RAW_MODULE_NAME "bus_managers/i2c/bus/raw/device_v1"


class I2CBus;
class I2CDevice;

extern device_manager_info *gDeviceManager;
extern struct device_module_info gI2CBusRawModule;
extern i2c_bus_interface gI2CBusModule;
extern i2c_device_interface gI2CDeviceModule;

class I2CDevice {
public:
								I2CDevice(device_node* node, I2CBus* bus,
									i2c_addr slaveAddress);
								~I2CDevice();

			status_t			InitCheck();

			status_t			ExecCommand(i2c_op op, const void *cmdBuffer,
									size_t cmdLength, void* dataBuffer,
									size_t dataLength);
			status_t			AcquireBus();
			void				ReleaseBus();

private:
			device_node*		fNode;
			I2CBus* 			fBus;
			i2c_addr 			fSlaveAddress;
};


class I2CBus {
public:
								I2CBus(device_node *node, uint8 id);
								~I2CBus();

			status_t			InitCheck();

			status_t			ExecCommand(i2c_op op, i2c_addr slaveAddress,
									const void *cmdBuffer, size_t cmdLength,
									void* dataBuffer, size_t dataLength);
			status_t			Scan();
			status_t			RegisterDevice(i2c_addr slaveAddress,
									char* hid, char** cid,
									acpi_handle acpiHandle);
			status_t			AcquireBus();
			void				ReleaseBus();

private:
			device_node*		fNode;
			uint8				fID;
			i2c_sim_interface*	fController;
			void*				fCookie;
};



#endif // I2C_PRIVATE_H
