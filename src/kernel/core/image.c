/* Loader API for userspace and the kernel
** 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


// The image API is mostly a wrapper around the ELF loader in the kernel;
// but it also adds identifying images and the managing part.
// It might as well support different loaders in the future, loaded as a
// modules. The ELF loader will be always built-in, though. But the API
// for those has yet to be defined.

// The image_key structure is used to identify a loaded image.
// When we should load an image, we first stat() it, and check if
// we already have loaded that exact image first, by comparing the
// stat data with the image_key.

typedef struct image_key {
	dev_t	device;
	ino_t	inode;

	// ToDo: should we take the 64bit internal BFS time values like BeOS does?
	time_t	last_modified_time;
	time_t	create_time;
	off_t	size;
};

// team independent structure for images currently loaded by the kernel

typedef struct loaded_image {
	loaded_image	*next;
	image_type		type;

	char			name[MAX_SYS_PATH_LEN];
	image_key		key;
} image;


// team dependent structure for images

typedef struct team_image {
	team_image		*next;
	image_id		id;
	loaded_image	*image;

	int32			sequence;
	int32			init_order;

	addr			text;
	addr			data;
} team_image;


hash_table *gImages;


static uint32
image_hash(void *_image, void *_key, uint32 range)
{
}


static int
image_compare(void *_image, void *_key)
{
	loaded_image *image = (loaded_image *)*_image;

	return !memcpy(&image->key, _key, sizeof(image_key));
}


image_id
load_image(const char *path)
{
	return B_ERROR;
}


status_t
unload_image(image_id id)
{
	return B_ERROR;
}


//	#pragma mark -
//	Functions exported for the user space


status_t
user_get_image_info(image_id id, image_info *userInfo, size_t size)
{
	image_info info;

	if (size != sizeof(image_info))
		return B_BAD_VALUE;

	if (!CHECK_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	// ToDo: do some real work here!

	if (user_memcpy(userInfo, &info, size) < B_OK)
		return B_BAD_ADDRESS;

	return B_ERROR;
}


status_t
user_get_next_image_info(team_id teamID, int32 *_cookie, image_info *userInfo, size_t size)
{
	image_info info;

	if (size != sizeof(image_info))
		return B_BAD_VALUE;

	if (!CHECK_USER_ADDRESS(userInfo) || !CHECK_USER_ADDRESS(_cookie))
		return B_BAD_ADDRESS;

	// ToDo: do some real work here!

	if (user_memcpy(userInfo, &info, size) < B_OK)
		return B_BAD_ADDRESS;

	return B_ERROR;
}


status_t
user_get_image_symbol(image_id id, const char *symbolName, int32 symbolType, void **_location)
{
	if (!CHECK_USER_ADDRESS(symbolName) || !CHECK_USER_ADDRESS(_location))
		return B_BAD_ADDRESS;

	return B_ERROR;
}


status_t
user_get_nth_image_symbol(image_id id, int32 n, char *name, size_t *_nameLength,
	int32 *_symbolType, void **_location)
{
	if (!CHECK_USER_ADDRESS(name)
		|| !CHECK_USER_ADDRESS(_nameLength)
		|| !CHECK_USER_ADDRESS(_symbolType)
		|| !CHECK_USER_ADDRESS(_location))
		return B_BAD_ADDRESS;

	return B_ERROR;
}


image_id
user_load_add_on(const char *userPath)
{
	if (!CHECK_USER_ADDRESS(userPath))
		return B_BAD_ADDRESS;

	return B_ERROR;
}


status_t
user_unload_add_on(image_id id)
{
	return B_ERROR;
}


thread_id
user_load_image(int argc, const char **argv, const char **env)
{
	if (!CHECK_USER_ADDRESS(argv) || !CHECK_USER_ADDRESS(env))
		return B_BAD_ADDRESS;

	return B_ERROR;
}

