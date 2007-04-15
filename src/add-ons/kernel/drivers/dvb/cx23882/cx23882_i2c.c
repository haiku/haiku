/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "cx23882_i2c.h"
#include "i2c_core.h"


static void
set_scl(void *cookie, int state)
{
	cx23882_device * device = cookie;
	if (state)
		device->i2c_reg |= I2C_SCL;
	else
		device->i2c_reg &= ~I2C_SCL;
	reg_write32(REG_I2C_CONTROL, device->i2c_reg);
	reg_read32(REG_I2C_CONTROL); // PCI bridge flush
}


static void
set_sda(void *cookie, int state)
{
	cx23882_device * device = cookie;
	if (state)
		device->i2c_reg |= I2C_SDA;
	else
		device->i2c_reg &= ~I2C_SDA;
	reg_write32(REG_I2C_CONTROL, device->i2c_reg);
	reg_read32(REG_I2C_CONTROL); // PCI bridge flush
}


static int
get_scl(void *cookie)
{
	cx23882_device * device = cookie;
	return (reg_read32(REG_I2C_CONTROL) & I2C_SCL) >> 1; // I2C_SCL is 0x02
}


static int
get_sda(void *cookie)
{
	cx23882_device * device = cookie;
	return reg_read32(REG_I2C_CONTROL) & I2C_SDA; // I2C_SDA is 0x01
}


status_t
i2c_init(cx23882_device *device)
{
	device->i2c_bus = i2c_create_bus(device, 80000, 2000000, set_scl, set_sda, get_scl, get_sda);
	device->i2c_reg = reg_read32(REG_I2C_CONTROL);
	device->i2c_reg &= ~I2C_HW_MODE;
	device->i2c_reg |= I2C_SCL | I2C_SDA;
	reg_write32(REG_I2C_CONTROL, device->i2c_reg);
	reg_read32(REG_I2C_CONTROL); // PCI bridge flush
	return device->i2c_bus ? B_OK : B_ERROR;
}


void
i2c_terminate(cx23882_device *device)
{
	i2c_delete_bus(device->i2c_bus);
}


