Disk driver ioctls
==================

Here is a list of ioctls usually implemented by disk devices.

B_GET_DEVICE_SIZE
-----------------

The parameter is a size_t and is filled with the disk size in bytes.
This is limited to 4GB and not very useful. B_GET_GEOMETRY is used instead.

B_GET_GEOMETRY
--------------

The parameter is a device_geometry structure to be filled with the device geometry.

B_GET_ICON_NAME
---------------

Deprecated. Get the name of an icon to use. The icons are hardcoded in Tracker.

B_GET_VECTOR_ICON
-----------------

The parameter is a device_icon structure to be populated with the icon data in HVIF format.
This icon is then used to show the disk in Tracker, for example.

B_EJECT_DEVICE
--------------

Eject the device (for removable devices).

B_LOAD_MEDIA
------------

Load the device (reverse of eject) if possible.

B_FLUSH_DRIVE_CACHE
-------------------

Make sure all data is stored on persistent storage and not in caches (including any caching inside
the device)

B_TRIM_DEVICE
-------------

The parameter is an fs_trim_data structure. It is guaranteed to be in kernel memory because
the partition manager pre-processes requests coming from userland and makes sure no sectors
are outside the partition range for a specific partition device.

Mark the listed areas on disk as unused, allowing future reads to these areas to return
random data or read errors. Flash memory devices (SSD, MMC, ...) may use this information
to optimize their internal storage.
