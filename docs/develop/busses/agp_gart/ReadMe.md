AGP (and PCI-express) Graphics Address Re-Mapping Table
=======================================================

The GART is an IO-MMU allowing the videocard and CPU to share some memory.
Either the CPU can access the video RAM directly ("aperture"), or the video
card can access the system RAM using DMA access.

The GART converts between physical addresses and virtual addresses on the
video card side. Of course, the CPU must then map these physical addresses
in its own address space to use them (using the MMU).

The GART works as you'd expect from an MMU. It has a page table (called GTT)
in RAM and walks it to figure out mappings. Since there cannot be page misses
(that would require exception handling on the GPU side), access to missing
pages are instead sent to a dedicated "scratch" page which is not used for
anything else.

Our driver implements the GART and GTT for Intel graphics card only, so far.
Since our videodrivers are only doing modesetting, they do not need much
support and other drivers implemented GTT management directly on their own
(it is usually enough to make the framebuffer accessible to the CPU). However,
this could be generalized into a more flexible iommu bus protocol.
