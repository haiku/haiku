/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#define sCommPageArea sCommPageCompatArea
#define sCommPageAddress sCommPageCompatAddress
#define sFreeCommPageSpace sFreeCommPageCompatSpace
#define sCommPageImage sCommPageCompatImage

#define allocate_commpage_entry allocate_commpage_compat_entry
#define fill_commpage_entry fill_commpage_compat_entry
#define get_commpage_image get_commpage_compat_image
#define clone_commpage_area clone_commpage_compat_area
#define commpage_init commpage_compat_init
#define commpage_init_post_cpus commpage_compat_init_post_cpus
#define arch_commpage_init arch_commpage_compat_init
#define arch_commpage_init_post_cpus arch_commpage_compat_init_post_cpus

#define ADDRESS_TYPE uint32
#define COMMPAGE_COMPAT

#include "commpage.cpp"
