/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#ifndef _ET6000GENERIC_H_
#define _ET6000GENERIC_H_

#include <Accelerant.h>


/*****************************************************************************/
status_t INIT_ACCELERANT(int fd);
ssize_t ACCELERANT_CLONE_INFO_SIZE(void);
void GET_ACCELERANT_CLONE_INFO(void *data);
status_t CLONE_ACCELERANT(void *data);
void UNINIT_ACCELERANT(void);
status_t GET_ACCELERANT_DEVICE_INFO(accelerant_device_info *adi);

uint32 ACCELERANT_MODE_COUNT(void);
status_t GET_MODE_LIST(display_mode *dm);
status_t PROPOSE_DISPLAY_MODE(display_mode *target, const display_mode *low, const display_mode *high);
status_t SET_DISPLAY_MODE(display_mode *mode_to_set);
status_t GET_DISPLAY_MODE(display_mode *current_mode);
status_t GET_FRAME_BUFFER_CONFIG(frame_buffer_config *a_frame_buffer);
status_t GET_PIXEL_CLOCK_LIMITS(display_mode *dm, uint32 *low, uint32 *high);

uint32 ACCELERANT_ENGINE_COUNT(void);
status_t ACQUIRE_ENGINE(uint32 capabilities, uint32 max_wait, sync_token *st, engine_token **et);
status_t RELEASE_ENGINE(engine_token *et, sync_token *st);
void WAIT_ENGINE_IDLE(void);
status_t GET_SYNC_TOKEN(engine_token *et, sync_token *st);
status_t SYNC_TO_TOKEN(sync_token *st);

void SCREEN_TO_SCREEN_BLIT(engine_token *et, blit_params *list, uint32 count);
void FILL_RECTANGLE(engine_token *et, uint32 color, fill_rect_params *list, uint32 count);


status_t createModesList(void);
void et6000aclInit(uint8 bpp);
void et6000aclWaitIdle(void);
/*****************************************************************************/


#endif /* _ET6000GENERIC_H_ */
