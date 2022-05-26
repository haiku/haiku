BitmapManager class
###################

The BitmapManager object handles all ServerBitmap allocation and
deallocation. The rest of the server uses CreateBitmap and DeleteBitmap
instead of new and delete.

Member Functions
================

BitmapManager(void)
-------------------

1. Create the bitmap list
2. Create the bitmap area
3. Allocate the access semaphore
4. Call set_buffer_area_management
5. Set up the buffer pool via bpool

~BitmapManager(void)
--------------------

1. Iterate over each item in the bitmap list, removing each item,
   calling brel() on its buffer, and deleting it.
2. Delete the bitmap list
3. Delete the bitmap area
4. Free the access semaphore

ServerBitmap \*CreateBitmap(BRect bounds, color_space space, int32 flags, int32 bytes_per_row=-1, screen_id screen=B_MAIN_SCREEN_ID)
------------------------------------------------------------------------------------------------------------------------------------

CreateBitmap is called by outside objects to allocate a ServerBitmap
object. If a problem occurs, it returns NULL.

1. Acquire the access semaphore
2. Verify parameters and if any are invalid, spew an error to stderr and
   return NULL
3. Allocate a new ServerBitmap
4. Allocate a buffer for the bitmap with the bitmap's
   theoretical buffer length
5. If NULL, delete the bitmap and return NULL
6. Set the bitmap's area and buffer to the appropriate values (area_for
   buffer and buffer)
7. Add the bitmap to the bitmap list
8. Release the access semaphore
9. Return the bitmap

void DeleteBitmap(ServerBitmap \*bitmap)
----------------------------------------

Frees a ServerBitmap allocated by CreateBitmap()

1. Acquire the access semaphore
2. Find the bitmap in the list
3. Remove the bitmap from the list or release the semaphore and return
   if not found
4. call brel() on the bitmap's buffer if it is non-NULL
5. delete the bitmap
6. Release the access semaphore

