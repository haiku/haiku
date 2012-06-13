/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*! User Runtime Loader support in the kernel */


#include <KernelExport.h>

#include <kernel.h>
#include <kimage.h>
#include <kscheduler.h>
#include <lock.h>
#include <Notifications.h>
#include <team.h>
#include <thread.h>
#include <thread_types.h>
#include <user_debugger.h>
#include <util/AutoLock.h>

#include <stdlib.h>
#include <string.h>


//#define TRACE_IMAGE
#ifdef TRACE_IMAGE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define ADD_DEBUGGER_COMMANDS


struct ImageTableDefinition {
	typedef image_id		KeyType;
	typedef struct image	ValueType;

	size_t HashKey(image_id key) const { return key; }
	size_t Hash(struct image* value) const { return value->info.id; }
	bool Compare(image_id key, struct image* value) const
		{ return value->info.id == key; }
	struct image*& GetLink(struct image* value) const
		{ return value->hash_link; }
};

typedef BOpenHashTable<ImageTableDefinition> ImageTable;


class ImageNotificationService : public DefaultNotificationService {
public:
	ImageNotificationService()
		: DefaultNotificationService("images")
	{
	}

	void Notify(uint32 eventCode, struct image* image)
	{
		char eventBuffer[128];
		KMessage event;
		event.SetTo(eventBuffer, sizeof(eventBuffer), IMAGE_MONITOR);
		event.AddInt32("event", eventCode);
		event.AddInt32("image", image->info.id);
		event.AddPointer("imageStruct", image);

		DefaultNotificationService::Notify(event, eventCode);
	}
};


static image_id sNextImageID = 1;
static mutex sImageMutex = MUTEX_INITIALIZER("image");
static ImageTable* sImageTable;
static ImageNotificationService sNotificationService;


/*!	Registers an image with the specified team.
*/
image_id
register_image(Team *team, image_info *_info, size_t size)
{
	image_id id = atomic_add(&sNextImageID, 1);
	struct image *image;

	image = (struct image*)malloc(sizeof(struct image));
	if (image == NULL)
		return B_NO_MEMORY;

	memcpy(&image->info, _info, sizeof(image_info));
	image->team = team->id;

	mutex_lock(&sImageMutex);

	image->info.id = id;

	// Add the app image to the head of the list. Some code relies on it being
	// the first image to be returned by get_next_image_info().
	if (image->info.type == B_APP_IMAGE)
		list_add_link_to_head(&team->image_list, image);
	else
		list_add_item(&team->image_list, image);
	sImageTable->Insert(image);

	// notify listeners
	sNotificationService.Notify(IMAGE_ADDED, image);

	mutex_unlock(&sImageMutex);

	TRACE(("register_image(team = %p, image id = %ld, image = %p\n", team, id, image));
	return id;
}


/*!	Unregisters an image from the specified team.
*/
status_t
unregister_image(Team *team, image_id id)
{
	status_t status = B_ENTRY_NOT_FOUND;

	mutex_lock(&sImageMutex);

	struct image *image = sImageTable->Lookup(id);
	if (image != NULL && image->team == team->id) {
		list_remove_link(image);
		sImageTable->Remove(image);
		status = B_OK;
	}

	mutex_unlock(&sImageMutex);

	if (status == B_OK) {
		// notify the debugger
		user_debug_image_deleted(&image->info);

		// notify listeners
		sNotificationService.Notify(IMAGE_REMOVED, image);

		free(image);
	}

	return status;
}


/*!	Counts the registered images from the specified team.
	Interrupts must be enabled.
*/
int32
count_images(Team *team)
{
	struct image *image = NULL;
	int32 count = 0;

	MutexLocker locker(sImageMutex);

	while ((image = (struct image*)list_get_next_item(&team->image_list, image))
			!= NULL) {
		count++;
	}

	return count;
}


/*!	Removes all images from the specified team. Must only be called
	with a team that has already been removed from the list (in thread_exit()).
*/
status_t
remove_images(Team *team)
{
	struct image *image;

	ASSERT(team != NULL);

	mutex_lock(&sImageMutex);

	while ((image = (struct image*)list_remove_head_item(&team->image_list))
			!= NULL) {
		sImageTable->Remove(image);
		free(image);
	}

	mutex_unlock(&sImageMutex);

	return B_OK;
}


status_t
_get_image_info(image_id id, image_info *info, size_t size)
{
	if (size > sizeof(image_info))
		return B_BAD_VALUE;

	status_t status = B_ENTRY_NOT_FOUND;

	mutex_lock(&sImageMutex);

	struct image *image = sImageTable->Lookup(id);
	if (image != NULL) {
		memcpy(info, &image->info, size);
		status = B_OK;
	}

	mutex_unlock(&sImageMutex);

	return status;
}


status_t
_get_next_image_info(team_id teamID, int32 *cookie, image_info *info,
	size_t size)
{
	if (size > sizeof(image_info))
		return B_BAD_VALUE;

	// get the team
	Team* team = Team::Get(teamID);
	if (team == NULL)
		return B_BAD_TEAM_ID;
	BReference<Team> teamReference(team, true);

	// iterate through the team's images
	MutexLocker imageLocker(sImageMutex);

	struct image* image = NULL;
	int32 count = 0;

	while ((image = (struct image*)list_get_next_item(&team->image_list,
			image)) != NULL) {
		if (count == *cookie) {
			memcpy(info, &image->info, size);
			(*cookie)++;
			return B_OK;
		}
		count++;
	}

	return B_ENTRY_NOT_FOUND;
}


#ifdef ADD_DEBUGGER_COMMANDS
static int
dump_images_list(int argc, char **argv)
{
	struct image *image = NULL;
	Team *team;

	if (argc > 1) {
		team_id id = strtol(argv[1], NULL, 0);
		team = team_get_team_struct_locked(id);
		if (team == NULL) {
			kprintf("No team with ID %" B_PRId32 " found\n", id);
			return 1;
		}
	} else
		team = thread_get_current_thread()->team;

	kprintf("Registered images of team %" B_PRId32 "\n", team->id);
	kprintf("    ID text       size    data       size    name\n");

	while ((image = (struct image*)list_get_next_item(&team->image_list, image))
			!= NULL) {
		image_info *info = &image->info;

		kprintf("%6" B_PRId32 " %p %-7" B_PRId32 " %p %-7" B_PRId32 " %s\n",
			info->id, info->text, info->text_size, info->data, info->data_size,
			info->name);
	}

	return 0;
}
#endif


struct image*
image_iterate_through_images(image_iterator_callback callback, void* cookie)
{
	MutexLocker locker(sImageMutex);

	ImageTable::Iterator it = sImageTable->GetIterator();
	struct image* image = NULL;
	while ((image = it.Next()) != NULL) {
		if (callback(image, cookie))
			break;
	}

	return image;
}


status_t
image_debug_lookup_user_symbol_address(Team *team, addr_t address,
	addr_t *_baseAddress, const char **_symbolName, const char **_imageName,
	bool *_exactMatch)
{
	// TODO: work together with ELF reader and runtime_loader

	struct image *image = NULL;

	while ((image = (struct image*)list_get_next_item(&team->image_list, image))
			!= NULL) {
		image_info *info = &image->info;

		if ((address < (addr_t)info->text
				|| address >= (addr_t)info->text + info->text_size)
			&& (address < (addr_t)info->data
				|| address >= (addr_t)info->data + info->data_size))
			continue;

		// found image
		*_symbolName = NULL;
		*_imageName = info->name;
		*_baseAddress = (addr_t)info->text;
		*_exactMatch = false;

		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
image_init(void)
{
	sImageTable = new(std::nothrow) ImageTable;
	if (sImageTable == NULL) {
		panic("image_init(): Failed to allocate image table!");
		return B_NO_MEMORY;
	}

	status_t error = sImageTable->Init();
	if (error != B_OK) {
		panic("image_init(): Failed to init image table: %s", strerror(error));
		return error;
	}

	new(&sNotificationService) ImageNotificationService();

#ifdef ADD_DEBUGGER_COMMANDS
	add_debugger_command("team_images", &dump_images_list, "Dump all registered images from the current team");
#endif

	return B_OK;
}


static void
notify_loading_app(status_t result, bool suspend)
{
	Team* team = thread_get_current_thread()->team;

	TeamLocker teamLocker(team);

	if (team->loading_info) {
		// there's indeed someone waiting
		struct team_loading_info* loadingInfo = team->loading_info;
		team->loading_info = NULL;

		loadingInfo->result = result;
		loadingInfo->done = true;

		// we're done with the team stuff, get the scheduler lock instead
		teamLocker.Unlock();
		InterruptsSpinLocker schedulerLocker(gSchedulerLock);

		// wake up the waiting thread
		if (loadingInfo->thread->state == B_THREAD_SUSPENDED)
			scheduler_enqueue_in_run_queue(loadingInfo->thread);

		// suspend ourselves, if desired
		if (suspend) {
			thread_get_current_thread()->next_state = B_THREAD_SUSPENDED;
			scheduler_reschedule();
		}
	}
}


//	#pragma mark -
//	Functions exported for the user space


status_t
_user_unregister_image(image_id id)
{
	return unregister_image(thread_get_current_thread()->team, id);
}


image_id
_user_register_image(image_info *userInfo, size_t size)
{
	image_info info;

	if (size != sizeof(image_info))
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userInfo)
		|| user_memcpy(&info, userInfo, size) < B_OK)
		return B_BAD_ADDRESS;

	return register_image(thread_get_current_thread()->team, &info, size);
}


void
_user_image_relocated(image_id id)
{
	image_info info;
	status_t error;

	// get an image info
	error = _get_image_info(id, &info, sizeof(image_info));
	if (error != B_OK) {
		dprintf("_user_image_relocated(%" B_PRId32 "): Failed to get image "
			"info: %" B_PRIx32 "\n", id, error);
		return;
	}

	// notify the debugger
	user_debug_image_created(&info);

	// If the image is the app image, loading is done. We need to notify the
	// thread who initiated the process and is now waiting for us to be done.
	if (info.type == B_APP_IMAGE)
		notify_loading_app(B_OK, true);
}


void
_user_loading_app_failed(status_t error)
{
	if (error >= B_OK)
		error = B_ERROR;

	notify_loading_app(error, false);

	_user_exit_team(error);
}


status_t
_user_get_image_info(image_id id, image_info *userInfo, size_t size)
{
	image_info info;
	status_t status;

	if (size > sizeof(image_info))
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	status = _get_image_info(id, &info, sizeof(image_info));

	if (user_memcpy(userInfo, &info, size) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_get_next_image_info(team_id team, int32 *_cookie, image_info *userInfo,
	size_t size)
{
	image_info info;
	status_t status;

	if (size > sizeof(image_info))
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userInfo) || !IS_USER_ADDRESS(_cookie))
		return B_BAD_ADDRESS;

	status = _get_next_image_info(team, _cookie, &info, sizeof(image_info));

	if (user_memcpy(userInfo, &info, size) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}

