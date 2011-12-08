/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_CONNECTOR_H
#define RADEON_HD_CONNECTOR_H

status_t gpio_probe();
status_t connector_attach_gpio(uint32 id, uint8 hw_line);
bool connector_read_edid(uint32 connector, edid1_info *edid);

#endif
