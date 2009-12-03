/*
 * Copyright 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 *
 * All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ALI_HARDWARE_H
#define _ALI_HARDWARE_H


status_t hardware_init(ali_dev *card);
void hardware_terminate(ali_dev *card);

uint32 ali_read_int(ali_dev *card);

bool ali_voice_start(ali_stream *stream);
void ali_voice_stop(ali_stream *stream);
void ali_voice_clr_int(ali_stream *stream);

#endif // _ALI_HARDWARE_H
