/*
 * Copyright 2006, Marcus Overhagen <marcus@overhagen.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

extern "C" uint16 call_pxe_bios(void *pxe, uint16 opcode, void *param);

void pxe_undi_init();
