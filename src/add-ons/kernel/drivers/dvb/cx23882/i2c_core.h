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

#ifndef __I2C_CORE_H
#define __I2C_CORE_H

#include <SupportDefs.h>

typedef void (*i2c_set_scl)(void *cookie, int state);
typedef void (*i2c_set_sda)(void *cookie, int state);
typedef int  (*i2c_get_scl)(void *cookie);
typedef int  (*i2c_get_sda)(void *cookie);

struct _i2c_bus;
typedef struct _i2c_bus i2c_bus;

i2c_bus *i2c_create_bus(void *cookie,
						int freqency,
						bigtime_t timeout,
						i2c_set_scl set_scl,
						i2c_set_sda set_sda,
						i2c_get_scl get_scl,
						i2c_get_sda get_sda);

void i2c_delete_bus(i2c_bus *bus);

status_t i2c_read(i2c_bus *bus, int address, void *data, int size);
status_t i2c_write(i2c_bus *bus, int address, const void *data, int size);

status_t i2c_xfer(i2c_bus *bus, int address,
				  const void *write_data, int write_size,
				  void *read_data, int read_size);

#endif
