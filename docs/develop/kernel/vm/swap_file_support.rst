Swap file
#######################

:hrev: hrev59537

This section describes how to use swap files in Haiku, and how the swap system
works.

How to use a swap file?
=======================

Like BeOS, Haiku uses "/var/swap" as default swap file. It is created
during the boot process and its size is the size of physical memory by
default, unless there's under 1GB of physical memory, in which case it's
double that. You can change its size through the VirtualMemory preference
application and your settings will take effect after restarting the system.

The default swap file "/var/swap" may not satisfy your need. Haiku internally
supports adding/removing a swap file dynamically, but this functionality
isn't exposed to userspace yet.

How does the swap system work?
==============================

The virtual memory subsystem of Haiku is similar to that of FreeBSD,
therefore our swap system implementation was inspired by FreeBSD's.

A swap system has two main functions: (1) maintain a map between anonymous
pages and swap space, so we can page in/out when needed. (2) manage the
allocation/deallocation of swap space. Let's see how these are implemented in
Haiku.

In order to maintain a map between pages and swap space, we need to record
the pages' swap address somewhere. Here we use swap blocks. A "swap_block"
structure contains swap address information for `SWAP_BLOCK_PAGES` (32)
consecutive pages from a same cache. So whenever we look for a page in swap
files, we should get the swap block for it. But how to get the swap block?
Here we use a hash table. All swap blocks in the system are arranged into a global
hash table. The hash table uses a cache's address and page index in this cache
as the hash key.

.. code-block:: text

                    ___________________________________________________________
    sSwapHashTable  |__________|___NULL___|___NULL___|___________|____NULL____|
                         |                                 |
                         |                                 |
                      ___V___                           ___V___
    swap_block   /----|__0__|                  /--------|__5__|
                 |    |__3__|--------\         |    /---|__6__|
                 |    |_..._|        |         |    |   |_..._|
                 |    |__2__|----\   |         |    |   |__20_|--------------->
                 |               |   |         |    |
                 |  _____________V___V_________V____V_________________________
    swap_file    `->|slot|slot|slot|slot|slot|slot|slot|slot|slot|slot|....|
                    |_0__|_1__|_2__|_3__|_4__|_5__|_6__|_7__|_8__|_9__|____|__


Here is an example. Suppose a page has been written out to swap space and now
its cache wants to read it in. It works as follows: look up the swap hash table
using address of the cache and page index as hash key, if successful, we get
the swap block containing the this page's swap address. Then search the swap
block to get the exact swap address of this page. After that, we can read the
page from swap file using vfs functions.

The swap system also manages allocation/deallocation of swap space. In our
implementation, each swap file is divided into page-sized slots (called "swap
pages") and a swap file can be seen as an array of many swap pages (see the
above diagram). Swap page is the unit for swap space allocation/deallocation
and we use swap page index (slot index) as swap space address instead of offset.
All the swap pages in the system are given a unified address and we leave one
page gap between two swap files. (e.g. there are 3 swap files in the system,
each has 100 swap pages, the address range (to be exact, page index) for each
swap file is: 0-99, 101-200, 202-301) Why leave a page gap between swap files?
Because in this way, we can easily tell if two adjacent pages are in the same
swap file. (See the code in `VMAnonymousCache::Read()`).

The efficiency of the FreeBSD swap system lies in a special data structure:
radix bitmap (i.e. bitmap using radix tree for hinting.) It can operate well no
matter how much fragmentation there is and no matter how large a bitmap is
used. FreeBSD's radix bitmap structure was ported to Haiku and used for
the same purpose.

Swap space allocation takes place when we write anonymous pages out. Since
all VMAnonymousCaches reserve their commitments from a global "memory and
swap" pool, this allocation should rarely (if ever) fail. Swap space
deallocation is triggered by the page daemon.
