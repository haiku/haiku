/*
 * "$Id: xmlppd.h,v 1.2 2007/12/24 03:05:52 rlk Exp $"
 *
 *   Copyright 2007 by Michael R Sweet and Robert Krawitz
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef GUTENPRINT_INTERNAL_XMLPPD_H
#define GUTENPRINT_INTERNAL_XMLPPD_H

extern stp_mxml_node_t *stpi_xmlppd_find_group_named(stp_mxml_node_t *root, const char *name);

extern stp_mxml_node_t *stpi_xmlppd_find_group_index(stp_mxml_node_t *root, int idx);

extern int stpi_xmlppd_find_group_count(stp_mxml_node_t *root);

extern stp_mxml_node_t *stpi_xmlppd_find_option_named(stp_mxml_node_t *root, const char *name);

extern stp_mxml_node_t *stpi_xmlppd_find_option_index(stp_mxml_node_t *root, int idx);

extern int stpi_xmlppd_find_option_count(stp_mxml_node_t *root);

extern stp_mxml_node_t *stpi_xmlppd_find_choice_named(stp_mxml_node_t *option, const char *name);

extern stp_mxml_node_t *stpi_xmlppd_find_choice_index(stp_mxml_node_t *option, int idx);

extern int stpi_xmlppd_find_choice_count(stp_mxml_node_t *option);

extern stp_mxml_node_t *stpi_xmlppd_find_page_size(stp_mxml_node_t *root, const char *name);

extern stp_mxml_node_t *stpi_xmlppd_read_ppd_file(const char *filename);

#endif /* GUTENPRINT_INTERNAL_XMLPPD_H */
