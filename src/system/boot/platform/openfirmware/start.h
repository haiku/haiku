/*
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2011, Alexander von Gluck, kallisti5@unixzen.com
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */


extern "C" void start(void *openFirmwareEntry);

void determine_machine(void);
void call_ctors(void);
