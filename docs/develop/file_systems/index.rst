Filesystem drivers
==================

Filesystem drivers are in src/add-ons/kernel/file_system

A filesystem usually relies on an underlying block device, but that's not
required. For example, NFS is a network filesystem, so it doesn't need one.

.. toctree::

   /file_systems/overview
   /file_systems/node_monitoring
   /file_systems/userlandfs
   /file_systems/ufs2
   /file_systems/xfs
   /file_systems/befs/resources
   /partitioning_systems/sun
