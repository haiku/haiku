/* User Runtime Loader support in the kernel
** 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <KernelExport.h>
#include <kimage.h>
#include <thread_types.h>
#include <thread.h>

#include <malloc.h>
#include <string.h>


// ToDo: register kernel images as well

struct image {
	struct image	*next;
	struct image	*prev;
	image_info		info;
};


static image_id gNextImageID = 1;


/** Registers an image with the specified team.
 */

image_id
register_image(team_id teamID, image_info *_info, size_t size)
{
	image_id id = atomic_add(&gNextImageID, 1);
	struct image *image;
	struct team *team;
	cpu_status state;

	image = malloc(sizeof(image_info));
	if (image == NULL)
		return B_NO_MEMORY;

	memcpy(&image->info, _info, sizeof(image_info));

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = team_get_team_struct_locked(teamID);
	if (team) {
		image->info.id = id;
		list_add_item(&team->image_list, image);
	}

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	dprintf("register_image(team = %ld, image id = %ld, image = %p\n", teamID, id, image);
	return id;
}


/** Unregisters an image from the specified team.
 */

status_t
unregister_image(team_id teamID, image_id id)
{
	status_t status = B_ENTRY_NOT_FOUND;
	struct team *team;
	cpu_status state;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = team_get_team_struct_locked(teamID);
	if (team) {
		struct image *image = NULL;

		while ((image = list_get_next_item(&team->image_list, image)) != NULL) {
			if (image->info.id == id) {
				list_remove_link(image);
				free(image);
				status = B_OK;
				break;
			}
		}
	}

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	return status;
}


/** Counts the registered images from the specified team.
 *	The team lock must be hold when you call this function.
 */

int32
count_images(struct team *team)
{
	struct image *image = NULL;
	int32 count = 0;

	while ((image = list_get_next_item(&team->image_list, image)) != NULL) {
		count++;
	}

	return count;
}


/** Removes all images from the specified team. Must only be called
 *	with the team lock hold or a team that has already been removed
 *	from the list (in thread_exit()).
 */

status_t
remove_images(struct team *team)
{
	struct image *image;

	ASSERT(team != NULL);

	while ((image = list_remove_head_item(&team->image_list)) != NULL) {
		free(image);
	}
	return B_OK;
}


status_t
_get_image_info(image_id id, image_info *info, size_t size)
{
	status_t status = B_ENTRY_NOT_FOUND;
	struct team *team;
	cpu_status state;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = thread_get_current_thread()->team;
	if (team) {
		struct image *image = NULL;

		while ((image = list_get_next_item(&team->image_list, image)) != NULL) {
			if (image->info.id == id) {
				memcpy(info, &image->info, size);
				status = B_OK;
				break;
			}
		}
	}

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	return status;
}


status_t
_get_next_image_info(team_id teamID, int32 *cookie, image_info *info, size_t size)
{
	status_t status = B_ENTRY_NOT_FOUND;
	struct team *team;
	cpu_status state;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = team_get_team_struct_locked(teamID);
	if (team) {
		struct image *image = NULL;
		int32 count = 0;

		while ((image = list_get_next_item(&team->image_list, image)) != NULL) {
			if (count == *cookie) {
				memcpy(info, &image->info, size);
				status = B_OK;
				(*cookie)++;
				break;
			}
			count++;
		}
	} else
		status = B_BAD_TEAM_ID;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	return status;
}


//	#pragma mark -
//	Functions exported for the user space


status_t
user_unregister_image(image_id id)
{
	return unregister_image(team_get_current_team_id(), id);
}


image_id
user_register_image(image_info *userInfo, size_t size)
{
	image_info info;
	
	if (size != sizeof(image_info))
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userInfo)
		|| user_memcpy(&info, userInfo, size) < B_OK)
		return B_BAD_ADDRESS;

	return register_image(team_get_current_team_id(), &info, size);
}


status_t
user_get_image_info(image_id id, image_info *userInfo, size_t size)
{
	image_info info;
	status_t status;

	if (size != sizeof(image_info))
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	status = _get_image_info(id, &info, size);

	if (user_memcpy(userInfo, &info, size) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
user_get_next_image_info(team_id team, int32 *_cookie, image_info *userInfo, size_t size)
{
	image_info info;
	status_t status;

	if (size != sizeof(image_info))
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userInfo) || !IS_USER_ADDRESS(_cookie))
		return B_BAD_ADDRESS;

	status = _get_next_image_info(team, _cookie, &info, size);

	if (user_memcpy(userInfo, &info, size) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}

