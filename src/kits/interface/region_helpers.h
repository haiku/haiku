#ifndef __REGION_HELPERS_H
#define __REGION_HELPERS_H

void zero_region(BRegion *a_region);
void clear_region(BRegion *a_region);
void cleanup_region_1(BRegion *region_in);
void cleanup_region(BRegion *region_in);
void sort_rects(clipping_rect *rects, long count);
void sort_trans(long *lptr1, long *lptr2, long count);
void cleanup_region_horizontal(BRegion *region_in);
void copy_region(BRegion *src_region, BRegion *dst_region);
void copy_region_n(BRegion*, BRegion*, long);
void and_region_complex(BRegion*, BRegion*, BRegion*);
void and_region_1_to_n(BRegion*, BRegion*, BRegion*);
void and_region(BRegion*, BRegion*, BRegion*);
void append_region(BRegion*, BRegion*, BRegion*);
void r_or(long, long, BRegion*, BRegion*, BRegion*, long*, long *);
void or_region_complex(BRegion*, BRegion*, BRegion*);
void or_region_1_to_n(BRegion*, BRegion*, BRegion*);
void or_region_no_x(BRegion*, BRegion*, BRegion*);
void or_region(BRegion*, BRegion*, BRegion*);
void sub_region_complex(BRegion*, BRegion*, BRegion*);
void r_sub(long , long, BRegion*, BRegion*, BRegion*, long*, long*);
void sub_region(BRegion*, BRegion*, BRegion*);

#endif // __REGION_HELPERS_H
