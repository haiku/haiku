/*
 * Copyright 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 *
 * Original code for memory management is
 * Copyright (c) 2003, Jerome Duval (jerome.duval@free.fr).
 *
 * All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef _UTIL_H
#define _UTIL_H

void *ali_mem_alloc(ali_dev *card, size_t size);
void ali_mem_free(ali_dev *card, void *ptr);

uint8 util_format_to_sample_size(uint32 format);
bool util_format_is_signed(uint32 format);
uint32 util_get_buffer_length_for_rate(uint32 rate);
uint32 util_sample_rate_in_bits(uint32 rate);

#endif // _UTIL_H
