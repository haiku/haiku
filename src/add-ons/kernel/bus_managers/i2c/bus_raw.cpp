/*
 * Copyright 2020, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "I2CPrivate.h"

#include <StackOrHeapArray.h>


static status_t
i2c_bus_raw_init(void* driverCookie, void **_cookie)
{
	CALLED();
	I2CBus *bus = (I2CBus*)driverCookie;
	TRACE("i2c_bus_raw_init bus %p\n", bus);

	*_cookie = bus;
	return B_OK;
}


static void
i2c_bus_raw_uninit(void *bus)
{
}


static status_t
i2c_bus_raw_open(void *bus, const char *path, int openMode,
	void **handle_cookie)
{
	CALLED();
	*handle_cookie = bus;
	TRACE("i2c_bus_raw_open bus %p\n", bus);

	return B_OK;
}


static status_t
i2c_bus_raw_close(void *cookie)
{
	return B_OK;
}


static status_t
i2c_bus_raw_free(void *cookie)
{
	return B_OK;
}


static status_t
i2c_bus_raw_control(void *_cookie, uint32 op, void *data, size_t length)
{
	CALLED();
	I2CBus *bus = (I2CBus*)_cookie;

	TRACE("i2c_bus_raw_control bus %p\n", bus);


	switch (op) {
		case I2CEXEC:
		{
			i2c_ioctl_exec exec;
			const void*	userCmdBuffer = NULL;
			void* userBuffer = NULL;
			if (user_memcpy(&exec, data, sizeof(i2c_ioctl_exec)) != B_OK)
				return B_BAD_ADDRESS;

			if (exec.cmdBuffer == NULL)
				exec.cmdLength = 0;
			if (exec.buffer == NULL)
				exec.bufferLength = 0;
			BStackOrHeapArray<uint8, 32> cmdBuffer(exec.cmdLength);
			BStackOrHeapArray<uint8, 32> buffer(exec.bufferLength);
			if (!cmdBuffer.IsValid() || !buffer.IsValid())
				return B_NO_MEMORY;

			if (exec.cmdBuffer != NULL) {
				if (!IS_USER_ADDRESS(exec.cmdBuffer)
					|| user_memcpy(cmdBuffer, exec.cmdBuffer, exec.cmdLength)
					!= B_OK) {
					return B_BAD_ADDRESS;
				}
				userCmdBuffer = exec.cmdBuffer;
				exec.cmdBuffer = cmdBuffer;
			}
			if (exec.buffer != NULL) {
				if (!IS_USER_ADDRESS(exec.buffer)
					|| user_memcpy(buffer, exec.buffer, exec.bufferLength)
					!= B_OK) {
					return B_BAD_ADDRESS;
				}
				userBuffer = exec.buffer;
				exec.buffer = buffer;
			}

			status_t status = bus->AcquireBus();
			if (status != B_OK)
				return status;

			status = bus->ExecCommand(exec.op, exec.addr,
				exec.cmdBuffer, exec.cmdLength, exec.buffer,
				exec.bufferLength);
			bus->ReleaseBus();

			if (status != B_OK)
				return status;

			exec.cmdBuffer = userCmdBuffer;
			if (exec.buffer != NULL) {
				if (user_memcpy(userBuffer, exec.buffer, exec.bufferLength)
					!= B_OK) {
					return B_BAD_ADDRESS;
				}
				exec.buffer = userBuffer;
			}

			if (user_memcpy(data, &exec, sizeof(i2c_ioctl_exec)) != B_OK)
				return B_BAD_ADDRESS;

			return B_OK;
		}
	}

	return B_ERROR;
}


static status_t
i2c_bus_raw_read(void *cookie, off_t position, void *data,
	size_t *numBytes)
{
	*numBytes = 0;
	return B_ERROR;
}


static status_t
i2c_bus_raw_write(void *cookie, off_t position,
	const void *data, size_t *numBytes)
{
	*numBytes = 0;
	return B_ERROR;
}


struct device_module_info gI2CBusRawModule = {
	{
		I2C_BUS_RAW_MODULE_NAME,
		0,
		NULL
	},

	i2c_bus_raw_init,
	i2c_bus_raw_uninit,
	NULL,	// removed

	i2c_bus_raw_open,
	i2c_bus_raw_close,
	i2c_bus_raw_free,
	i2c_bus_raw_read,
	i2c_bus_raw_write,
	NULL,	// io
	i2c_bus_raw_control,
	NULL,	// select
	NULL	// deselect
};
