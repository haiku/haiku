/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _KERNEL_IMAGE_H
#define _KERNEL_IMAGE_H


#include <image.h>


struct team;

extern image_id register_image(team_id team, image_info *info, size_t size);
extern status_t unregister_image(team_id team, image_id id);
extern int32 count_images(struct team *team);
extern status_t remove_images(struct team *team);

// user-space exported calls
extern status_t user_unregister_image(image_id id);
extern image_id user_register_image(image_info *userInfo, size_t size);
extern status_t user_get_next_image_info(team_id team, int32 *_cookie,
	image_info *userInfo, size_t size);
extern status_t user_get_image_info(image_id id, image_info *userInfo, size_t size);

#endif	/* _KRENEL_IMAGE_H */
