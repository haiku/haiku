#ifndef VIDEO_H
#define VIDEO_H


#include <SupportDefs.h>


#ifdef __cplusplus
class Menu;
Menu *video_mode_menu();

extern "C"
#endif
status_t video_init(void);

#endif	/* VIDEO_H */
