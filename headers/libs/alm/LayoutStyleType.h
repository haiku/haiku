/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#ifndef	LAYOUT_STYLE_TYPE_H
#define	LAYOUT_STYLE_TYPE_H


namespace BALM {

/**
 * The possibilities for adjusting a GUI's layout.
 * Either change the child controls so that they fit into their parent control, or adjust 
 * the size of the parent control so that the children have as much space as they want.
 */
enum LayoutStyleType {
	FIT_TO_SIZE, ADJUST_SIZE
};

}	// namespace BALM

using BALM::LayoutStyleType;

#endif
