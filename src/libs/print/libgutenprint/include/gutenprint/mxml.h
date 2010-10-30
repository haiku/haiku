/*
 * "$Id: mxml.h,v 1.2 2008/06/08 15:10:08 rlk Exp $"
 *
 * Header file for mini-XML, a small XML-like file parsing library.
 *
 * Copyright 2003 by Michael Sweet.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/**
 * @file gutenprint/mxml.h
 * @brief Mini-XML XML parsing functions.
 */

/*
 * Prevent multiple inclusion...
 */

#ifndef GUTENPRINT_MXML_H
#  define GUTENPRINT_MXML_H

/*
 * Include necessary headers...
 */

#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <ctype.h>
#  include <errno.h>


/*
 * Constants...
 */

#  define STP_MXML_WRAP		70	/* Wrap XML output at this column position */
#  define STP_MXML_TAB		8	/* Tabs every N columns */

#  define STP_MXML_NO_CALLBACK	0	/* Don't use a type callback */
#  define STP_MXML_NO_PARENT	0	/* No parent for the node */

#  define STP_MXML_DESCEND		1	/* Descend when finding/walking */
#  define STP_MXML_NO_DESCEND	0	/* Don't descend when finding/walking */
#  define STP_MXML_DESCEND_FIRST	-1	/* Descend for first find */

#  define STP_MXML_WS_BEFORE_OPEN	0	/* Callback for before open tag */
#  define STP_MXML_WS_AFTER_OPEN	1	/* Callback for after open tag */
#  define STP_MXML_WS_BEFORE_CLOSE	2	/* Callback for before close tag */
#  define STP_MXML_WS_AFTER_CLOSE	3	/* Callback for after close tag */

#  define STP_MXML_ADD_BEFORE	0	/* Add node before specified node */
#  define STP_MXML_ADD_AFTER	1	/* Add node after specified node */
#  define STP_MXML_ADD_TO_PARENT	NULL	/* Add node relative to parent */


/*
 * Data types...
 */

typedef enum stp_mxml_type_e		/**** The XML node type. ****/
{
  STP_MXML_ELEMENT,				/* XML element with attributes */
  STP_MXML_INTEGER,				/* Integer value */
  STP_MXML_OPAQUE,				/* Opaque string */
  STP_MXML_REAL,				/* Real value */
  STP_MXML_TEXT				/* Text fragment */
} stp_mxml_type_t;

typedef struct stp_mxml_attr_s		/**** An XML element attribute value. ****/
{
  char	*name;				/* Attribute name */
  char	*value;				/* Attribute value */
} stp_mxml_attr_t;

typedef struct stp_mxml_value_s		/**** An XML element value. ****/
{
  char		*name;			/* Name of element */
  int		num_attrs;		/* Number of attributes */
  stp_mxml_attr_t	*attrs;			/* Attributes */
} stp_mxml_element_t;

typedef struct stp_mxml_text_s		/**** An XML text value. ****/
{
  int		whitespace;		/* Leading whitespace? */
  char		*string;		/* Fragment string */
} stp_mxml_text_t;

typedef union stp_mxml_value_u		/**** An XML node value. ****/
{
  stp_mxml_element_t	element;	/* Element */
  int			integer;	/* Integer number */
  char			*opaque;	/* Opaque string */
  double		real;		/* Real number */
  stp_mxml_text_t		text;		/* Text fragment */
} stp_mxml_value_t;

typedef struct stp_mxml_node_s stp_mxml_node_t;	/**** An XML node. ****/

struct stp_mxml_node_s			/**** An XML node. ****/
{
  stp_mxml_type_t	type;			/* Node type */
  stp_mxml_node_t	*next;			/* Next node under same parent */
  stp_mxml_node_t	*prev;			/* Previous node under same parent */
  stp_mxml_node_t	*parent;		/* Parent node */
  stp_mxml_node_t	*child;			/* First child node */
  stp_mxml_node_t	*last_child;		/* Last child node */
  stp_mxml_value_t	value;			/* Node value */
};


/*
 * C++ support...
 */

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */

/*
 * Prototypes...
 */

extern void		stp_mxmlAdd(stp_mxml_node_t *parent, int where,
			        stp_mxml_node_t *child, stp_mxml_node_t *node);
extern void		stp_mxmlDelete(stp_mxml_node_t *node);
extern const char	*stp_mxmlElementGetAttr(stp_mxml_node_t *node, const char *name);
extern void		stp_mxmlElementSetAttr(stp_mxml_node_t *node, const char *name,
			                   const char *value);
extern stp_mxml_node_t	*stp_mxmlFindElement(stp_mxml_node_t *node, stp_mxml_node_t *top,
			                 const char *name, const char *attr,
					 const char *value, int descend);
extern stp_mxml_node_t	*stp_mxmlLoadFile(stp_mxml_node_t *top, FILE *fp,
			              stp_mxml_type_t (*cb)(stp_mxml_node_t *));
extern stp_mxml_node_t	*stp_mxmlLoadFromFile(stp_mxml_node_t *top, const char *file,
			              stp_mxml_type_t (*cb)(stp_mxml_node_t *));
extern stp_mxml_node_t	*stp_mxmlLoadString(stp_mxml_node_t *top, const char *s,
			                stp_mxml_type_t (*cb)(stp_mxml_node_t *));
extern stp_mxml_node_t	*stp_mxmlNewElement(stp_mxml_node_t *parent, const char *name);
extern stp_mxml_node_t	*stp_mxmlNewInteger(stp_mxml_node_t *parent, int integer);
extern stp_mxml_node_t	*stp_mxmlNewOpaque(stp_mxml_node_t *parent, const char *opaque);
extern stp_mxml_node_t	*stp_mxmlNewReal(stp_mxml_node_t *parent, double real);
extern stp_mxml_node_t	*stp_mxmlNewText(stp_mxml_node_t *parent, int whitespace,
			             const char *string);
extern void		stp_mxmlRemove(stp_mxml_node_t *node);
extern char		*stp_mxmlSaveAllocString(stp_mxml_node_t *node,
			        	     int (*cb)(stp_mxml_node_t *, int));
extern int		stp_mxmlSaveFile(stp_mxml_node_t *node, FILE *fp,
			             int (*cb)(stp_mxml_node_t *, int));
extern int		stp_mxmlSaveToFile(stp_mxml_node_t *node, const char *fp,
			             int (*cb)(stp_mxml_node_t *, int));
extern int		stp_mxmlSaveString(stp_mxml_node_t *node, char *buffer,
			               int bufsize,
			               int (*cb)(stp_mxml_node_t *, int));
extern stp_mxml_node_t	*stp_mxmlWalkNext(stp_mxml_node_t *node, stp_mxml_node_t *top,
			              int descend);
extern stp_mxml_node_t	*stp_mxmlWalkPrev(stp_mxml_node_t *node, stp_mxml_node_t *top,
			              int descend);


/*
 * C++ support...
 */

#  ifdef __cplusplus
}
#  endif /* __cplusplus */
#endif /* !GUTENPRINT_MXML_H */


/*
 * End of "$Id: mxml.h,v 1.2 2008/06/08 15:10:08 rlk Exp $".
 */
