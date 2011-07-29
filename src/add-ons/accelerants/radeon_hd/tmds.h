/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_TMDS_H
#define RADEON_HD_TMDS_H


void TMDSVoltageControl(uint8 tmdsIndex);
bool TMDSSense(uint8 tmdsIndex);
status_t TMDSPower(uint8 tmdsIndex, int command);
status_t TMDSSet(uint8 tmdsIndex, display_mode *mode);
void TMDSAllIdle();


#endif
