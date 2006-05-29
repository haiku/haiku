/*
 * This file is a part of SiS 7018 BeOS driver project.
 * Copyright (c) 2002-2003 by Siarzuk Zharski <imker@gmx.li>
 *
 * This file may be used under the terms of the BSD License
 * See the file "License" for details.
 */
 
#ifndef _MIDI_H_
 #define _MIDI_H_

#ifndef _DRIVERS_H
 #include <Drivers.h>
#endif //_DRIVERS_H

extern device_hooks midi_hooks;
void midi_interrupt_op(int32 op, void * data);
extern generic_mpu401_module *mpu401;
#endif //_MIDI_H_
