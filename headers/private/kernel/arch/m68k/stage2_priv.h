/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _STAGE2_PRIV_H
#define _STAGE2_PRIV_H

#define LOAD_ADDR 0x100000
#define BOOTDIR_ADDR 0x101000

int s2_text_init();
void s2_change_framebuffer_addr(unsigned int address);
void putchar(char c);
void puts(char *str);
int printf(const char *fmt, ...);

int of_init(void *of_entry);
int of_open(const char *node_name);
int of_finddevice(const char *dev);
int of_instance_to_package(int in_handle);
int of_getprop(int handle, const char *prop, void *buf, int buf_len);
int of_setprop(int handle, const char *prop, const void *buf, int buf_len);
int of_read(int handle, void *buf, int buf_len);
int of_write(int handle, void *buf, int buf_len);
int of_seek(int handle, long long pos);

int s2_mmu_init();
void mmu_map_page(unsigned int vsid, unsigned long pa, unsigned long va);;
void syncicache(void *address, int len);

void s2_faults_init(kernel_args *ka);

void getibats(int bats[8]);
void setibats(int bats[8]);
void getdbats(int bats[8]);
void setdbats(int bats[8]);
unsigned int *getsdr1();
void setsdr1(unsigned int sdr);
unsigned int getsr(int sr);
unsigned int getmsr();
void setmsr(unsigned int msr);

#endif
