/*
 * Copyright (c) 2002, Intel Corporation. All rights reserved.
 * Created by:  julie.n.fleischer REMOVE-THIS AT intel DOT com
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this
 * source tree.
 */

/*
 * return codes
 */
#define PTS_PASS        0
#define PTS_FAIL        1
#define PTS_UNRESOLVED  2
#define PTS_UNSUPPORTED 4
#define PTS_UNTESTED    5

/* colors */
const char* const normal = "\033[0m";
const char* const green = "\033[32m";
const char* const red = "\033[31m";

const char* const boldOn = "\033[1m";
const char* const boldOff = "\033[22m";
