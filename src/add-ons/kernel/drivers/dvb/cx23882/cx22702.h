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

#ifndef __CX22702_H
#define __CX22702_H

#include "i2c_core.h"
#include "dvb.h"

status_t cx22702_reg_write(i2c_bus *bus, uint8 reg, uint8 data);
status_t cx22702_reg_read(i2c_bus *bus, uint8 reg, uint8 *data);

status_t cx22702_init(i2c_bus *bus);

status_t cx22702_get_frequency_info(i2c_bus *bus, dvb_frequency_info_t *info);

status_t cx22702_set_tuning_parameters(i2c_bus *bus, const dvb_t_tuning_parameters_t *params);
status_t cx22702_get_tuning_parameters(i2c_bus *bus, dvb_t_tuning_parameters_t *params);

status_t cx22702_get_status(i2c_bus *bus, dvb_status_t *status);
status_t cx22702_get_ss(i2c_bus *bus, uint32 *ss);
status_t cx22702_get_ber(i2c_bus *bus, uint32 *ber);
status_t cx22702_get_snr(i2c_bus *bus, uint32 *snr);
status_t cx22702_get_upc(i2c_bus *bus, uint32 *upc);

#endif
