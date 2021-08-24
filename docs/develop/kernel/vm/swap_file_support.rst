Swap file
#######################

This section describes how to use swap file in Haiku and how the swap system 
works.

How to use a swap file?
=======================

Like BeOS, Haiku uses "/var/swap" as default swap file. It is created 
during the boot process and its size is twice the size of physical memory by 
default. You can change its size through the VirtualMemory preference 
application and your settings will take effect after restarting the system.

The default swap file "/var/swap" may not satisfy your need. Haiku allows 
adding/removing a swap file dynamically. (This is *NOT* implemented yet, since
I do not know how to add bin commands "swapon" and "swapoff" in the system. 
It needs to be done in the future.)

How swap system works?
======================

The virtual memory subsystem of Haiku is very similar to that of FreeBSD,
therefore our swap system implementation is borrowed from FreeBSD.

A swap system has two main functions: (1) maintain a map between anonymous
pages and swap space, so we can page in/out when needed. (2) manage the 
allocation/deallocation of swap space. Let's see how these are implemented in
Haiku.

In order to maintain a map between pages and swap space, we need to record
the pages' swap address somewhere. Here we use swap blocks. A "swap_block"
structure contains swap address information for 32 (value of SWAP_BLOCK_PAGES) 
consecutive pages from a same cache. So whenever we look for a page in swap 
files, we should get the swap block for it. But how to get the swap block? 
Here we use hash table. All swap blocks in the system are arranged into a global
hash table. The hash table uses a cache's address and page index in this cache 
as hash key.

Here is an example. Suppose a page has been paged out to swap space and now
its cache wants to page it in. It works as follows: look up the swap hash table
using address of the cache and page index as hash key, if successful, we get
the swap block containing the this page's swap address. Then search the swap
block to get the exact swap address of this page. After that, we can read the
page from swap file using vfs functions.

I draw a picture and hope it could help you understand the above words.

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


The swap system also manages allocation/deallocation of swap space.	In our
implementation, each swap file is divided into page-sized slots(called "swap 
pages") and a swap file can be seen as an array of many swap pages(see the 
above picture). Swap page is the unit for swap space allocation/deallocation 
and we use swap page index (slot index) as swap space address instead of offset.
All the swap pages in the system are given a unified address and we leave one 
page gap between two swap files. (e.g. there are 3 swap files in the system, 
each has 100 swap pages, the address range(to be exact, page index) for each 
swap file is: 0-99, 101-200, 202-301) Why leave a page gap between swap files?
Because in this way, we can easily tell if two adjacent pages are in a same 
swap file. (See the code in VMAnonymousCache::Read()).

The efficiency of the FreeBSD swap system lies in a special data structure:
radix bitmap(i.e. bitmap using radix tree for hinting.) It can operate well no
matter how much fragmentation there is and no matter how large a bitmap is 
used. I have ported the radix bitmap structure to Haiku, so our swap system 
will have a good performance. More information on radix bitmap, please look 
at the source code.

Swap space allocation takes place when we swap anonymous pages out.
In order to make the allocation less probable to fail, anonymous cache will
reserve swap space when it is initialized. If there is not enough swap space
left, physical memory will be reserved. Swap space deallocation happens when
available swap space is low. The page daemon will scan a number of pages and
if the scanned page has swap space assigned, its swap space will be freed.

Acknowledgement
---------------

Special thanks to my mentor Ingo. He is a knowledged person and always
gives me encouragement. Without his consistent and illuminating instructions,
this project would not have reached its present status.

If you find bugs or have suggestions for swap system, you can contact me 
via upczhsh@163.com. Thanks in advance.

Zhao Shuai - upczhsh@163.com - 2008-08-21
