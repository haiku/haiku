/*
 * Copyright 2013, Fredrik Homlqvist, fredrik.holmqvist@gmail.com.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef EFI_PLATFORM_H
#define EFI_PLATFORM_H


#include "efibind.h"
#include "efidef.h"
#include "efidevp.h"
#include "efiprot.h"
#include "eficon.h"
#include "efierr.h"
#include "efiapi.h"


extern const EFI_SYSTEM_TABLE		*kSystemTable;
extern const EFI_BOOT_SERVICES		*kBootServices;
extern const EFI_RUNTIME_SERVICES	*kRuntimeServices;
extern EFI_HANDLE kImage;

#endif	/* EFI_PLATFORM_H */
